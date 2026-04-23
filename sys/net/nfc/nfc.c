#include "macros/utils.h"
#include "net/nfc.h"

nfc_bitrate_t nfc_bitrate_select(
    nfc_bitrate_set_t set1,
    nfc_bitrate_set_t set2,
    nfc_bitrate_t current,
    nfc_bitrate_selection_strategy_t strategy
) {
    if (strategy == NFC_BITRATE_CHOOSE_FORCED) {
        // Make sure this set just consists of the enforced bitrate;
        assert(set1 == (nfc_bitrate_t)(1 << __builtin_ctz(set1)));
        return (nfc_bitrate_t)set1;
    }
    if (strategy == NFC_BITRATE_CHOOSE_CURRENT) {
        return current;
    }
    nfc_bitrate_set_t gcd = set1 & set2;
    if (gcd == 0) {
        return 0;
    }
    if (strategy == NFC_BITRATE_CHOOSE_FASTEST) {
        // Count leading zeroes, get fastest bit rate
        return (nfc_bitrate_t)(1 << ((sizeof(nfc_bitrate_set_t) * 8) - 1 - __builtin_clz(gcd)));
    } else {
        // Count trailing zeroes, get slowest bit rate
        return (nfc_bitrate_t)(1 << __builtin_ctz(gcd));
    }
}

void nfc_bitrate_select_bidirectional(
    const nfc_bidirectional_bitrate_selector_t* selector,
    nfc_bitrate_t* downstream, nfc_bitrate_t* upstream,
    nfc_bitrate_set_t downstream_supported, nfc_bitrate_set_t upstream_supported,
    bool require_symmetric
) {
    nfc_bitrate_selection_strategy_t downstream_strategy = selector->downstream.strategy;
    nfc_bitrate_selection_strategy_t upstream_strategy = selector->upstream.strategy;

    if (require_symmetric) {
        downstream_strategy = upstream_strategy = MAX(downstream_strategy, upstream_strategy);
        // This is what the other party supports, input should already satisfy this technically
        nfc_bitrate_set_t same = downstream_supported & upstream_supported;
        // Now we cross-intersect, such that the gcd calculation in nfc_bitrate_select
        // yields...
        //    selector->downstream.set & (same & selector->upstream.set)
        downstream_supported = same & selector->upstream.set;
        //    selector->upstream.set & (same & selector->downstream.set)
        upstream_supported = same & selector->downstream.set;
        // ... which is the same,
    }

    *downstream = nfc_bitrate_select(selector->downstream.set, downstream_supported,
        *downstream, downstream_strategy);
    *upstream = nfc_bitrate_select(selector->upstream.set, upstream_supported,
        *upstream, upstream_strategy);
}

uint16_t iso_dep_frame_sizes[] = { 16,24,32,40,48,64,96,128,356,512,1024,2048,4096 };

const nfc_a_polling_command_t wupa = NFC_A_FRAME_CODE_POLLING_ALL;
const nfc_a_polling_frame_t nfc_a_polling_frame_all = {
    .frame = &wupa,
    .length = { .bytes = 1, .trailing_bits = 7 }
};

const nfc_a_polling_command_t reqa = NFC_A_FRAME_CODE_POLLING_ONLY_AWAKE;
const nfc_a_polling_frame_t nfc_a_polling_frame_only_awake = {
    .frame = &reqa,
    .length = { .bytes = 1, .trailing_bits = 7 }
};

bool nfc_a_polling_filter_matches(const nfc_a_polling_filter_t* filter, const nfc_a_tag_t* tag) {
    assert(tag);
    if (!filter) {
        return true;
    }

    if (filter->polling_response.mask.uint != 0) {
        if (filter->polling_response.value.uint !=
            (tag->polling_response.uint & filter->polling_response.mask.uint)) {
            return false;
        }
    }

    if (filter->select_response.mask != 0) {
        if (filter->select_response.value !=
            (tag->select_response & filter->select_response.mask)) {
            return false;
        }
    }
    return false;
}

bool nfc_b_polling_filter_matches(const nfc_b_polling_filter_t* filter, const nfc_b_tag_t* tag) {
    assert(tag);
    if (!filter) {
        return true;
    }

    if (filter->id) {
        if (memcmp(filter->id, tag->polling_response.id, sizeof(nfc_b_id_t)) != 0) {
            return false;
        }
    }

    if (filter->application_family != 0) {
        if (filter->application_family != tag->polling_response.application_family) {
            return false;
        }
    }

    if (filter->aid_crc[0] != 0 || filter->aid_crc[1] != 0) {
        if (memcmp(filter->aid_crc, tag->polling_response.aid_crc, 2) != 0) {
            return false;
        }
    }

    if (filter->bitrates.raw != 0) {
        if (filter->bitrates.raw != (tag->polling_response.bitrates.raw & filter->bitrates.raw)) {
            return false;
        }
    }

    return
        (!filter->require_iso_dep_supported || tag->polling_response.iso_dep_supported) ||
        (!filter->require_cid_supported || tag->polling_response.cid_supported) ||
        (!filter->require_cid_supported || tag->polling_response.cid_supported) ||
        (!filter->require_nad_supported || tag->polling_response.nad_supported) ||
        (!filter->require_application_data_standardized || tag->polling_response.application_data_standardized);
}

bool nfc_f_polling_filter_matches(const nfc_f_polling_filter_t* filter, const nfc_f_tag_t* tag) {
    assert(tag);
    if (!filter) {
        return true;
    }

    if (filter->id) {
        if (memcmp(filter->id, &tag->id, sizeof(nfc_f_id_t)) != 0) {
            return false;
        }
    }

    if (filter->bitrates != 0 && tag->bitrates != 0) {
        if ((tag->bitrates & filter->bitrates) != filter->bitrates) {
            return false;
        }
    }

    if (filter->system_code_count > 0 && tag->system_code != 0) {
        for (size_t i = 0; i < filter->system_code_count; i += 1) {
            if (filter->system_codes[i] == tag->system_code) {
                return true;
            }
        }
        return false;
    }
    return true;
}

bool nfc_v_polling_filter_matches(const nfc_v_polling_filter_t* filter, const nfc_v_tag_t* tag) {
    assert(tag);
    if (!filter) {
        return true;
    }

    if (filter->id && filter->id_prefix_length > 0) {
        assert(filter->id_prefix_length <= sizeof(nfc_v_id_t));
        if (memcmp(filter->id, &tag->id, filter->id_prefix_length) != 0) {
            return false;
        }
    }

    if (filter->storage_format_identifier != 0 && tag->storage_format_identifier != 0) {
        if (filter->storage_format_identifier != tag->storage_format_identifier) {
            return false;
        }
    }
    return true;
}
