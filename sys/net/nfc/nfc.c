#include <stdio.h>
#include "errno.h"
#include "macros/utils.h"
#include "architecture.h"
#include "net/nfc.h"

nfc_bitrate_t nfc_bitrate_select(
    nfc_bitrate_set_t set1,
    nfc_bitrate_set_t set2,
    nfc_bitrate_t current,
    nfc_bitrate_selection_strategy_t strategy
) {
    printf("set1=%u\n", set1);
    printf("set2=%u\n", set2);
    if (strategy == NFC_BITRATE_CHOOSE_FORCED) {
        // Make sure this set just consists of the enforced bitrate;
        assert(set1 == (nfc_bitrate_t)(1 << __builtin_ctz(set1)));
        return (nfc_bitrate_t)set1;
    }
    if (strategy == NFC_BITRATE_CHOOSE_CURRENT) {
        return current;
    }
    nfc_bitrate_set_t gcd = set1 & set2;
    printf("gcd=%u\n", gcd);
    if (gcd == 0) {
        return 0;
    }
    if (strategy == NFC_BITRATE_CHOOSE_FASTEST) {
        // Count leading zeroes, get fastest bit rate
        printf("fastest=%u\n", nfc_bitrate_set_fastest(gcd));
        return nfc_bitrate_set_fastest(gcd);
    } else {
        printf("slowest=%u\n", nfc_bitrate_set_slowest(gcd));
        // Count trailing zeroes, get slowest bit rate
        return nfc_bitrate_set_slowest(gcd);
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

uint16_t iso_dep_frame_sizes[] = { 16,24,32,40,48,64,96,128,256,512,1024,2048,4096 };

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

ssize_t nfc_a_ats_parse(nfc_a_ats_t* dest, uint8_t** cursor, size_t length, size_t historical_capacity) {
    if (length < 1) {
        return -EBADMSG;
    }
    uint8_t ats_length = *(*cursor)++;
    if (ats_length < 1 || ats_length > length) {
        return -EBADMSG;
    }
    dest->length = ats_length;
    if (ats_length == 1) {
        return 1;
    }
    dest->t0 = *(*cursor)++;

    uint8_t info_length = dest->ta_present + dest->tb_present + dest->tc_present;
    if (ats_length < 2 + info_length) {
        return -EBADMSG;
    }
    size_t historical_length = ats_length - 2 - info_length;
    if (historical_length > historical_capacity) {
        return -ENOBUFS;
    }

    if (dest->ta_present) {
        dest->ta = *(*cursor)++;
    }

    if (dest->tb_present) {
        dest->tb = *(*cursor)++;
    }

    if (dest->tc_present) {
        dest->tc = *(*cursor)++;
    }

    if (historical_length > 0) {
        memcpy(dest->historical, *cursor, historical_length);
        *cursor += historical_length;
    }
    return (ssize_t)(size_t)(*cursor - (uint8_t*)dest);
}

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

const char* nfc_string_from_field_mode(nfc_field_mode_t field_mode) {
    assert(field_mode <= 1);
    switch (field_mode) {
        case NFC_FIELD_MODE_READER_WRITER_TAG: return "r/w-tag";
        case NFC_FIELD_MODE_PEERS: return "peer";
        default: UNREACHABLE();
    }
}

const char* nfc_string_from_role(nfc_role_t role) {
    assert(role <= 1);
    switch (role) {
        case NFC_ROLE_TARGET: return "target";
        case NFC_ROLE_INITIATOR: return "initiator";
        default: UNREACHABLE();
    }
}

void nfc_print_target(const nfc_target_t* target) {
    assert(target);
    printf("<target field_mode=%s polling={ br=%u }",
           nfc_string_from_field_mode(target->field_mode),
           nfc_bitrate_kbps(target->parameters.polling.bitrate));

    if (target->parameters.downstream.bitrate != NFC_BITRATE_UNSET ||
        target->parameters.downstream.max_packet_length != 0) {
        printf(" downstream={ ");
        if (target->parameters.downstream.bitrate != NFC_BITRATE_UNSET) {
            printf("br=%u ", nfc_bitrate_kbps(target->parameters.downstream.bitrate));
        }
        if (target->parameters.downstream.max_packet_length != 0) {
            printf("pktlen=%" PRIuSIZE " ", target->parameters.downstream.max_packet_length);
        }
        printf("}");
    }

    if (target->parameters.upstream.bitrate != NFC_BITRATE_UNSET ||
        target->parameters.upstream.max_packet_length != 0) {
        printf(" upstream={ ");
        if (target->parameters.upstream.bitrate != NFC_BITRATE_UNSET) {
            printf("br=%u ", nfc_bitrate_kbps(target->parameters.upstream.bitrate));
        }
        if (target->parameters.upstream.max_packet_length != 0) {
            printf("pktlen=%" PRIuSIZE " ", target->parameters.upstream.max_packet_length );
        }
        printf("}");
    }

    if (target->field_mode == NFC_FIELD_MODE_READER_WRITER_TAG) {
        printf(" tag=");
        nfc_print_tag(&target->tag);
    }

    if (target->higher_layer.nfc_dep.length > 0) {
        printf(" atr_res=");
        nfc_dep_print_target(&target->higher_layer.nfc_dep);
    }
    printf(">");
}

void nfc_print_tag(const nfc_tag_t* tag) {
    assert(tag);
    switch (tag->technology) {
        case NFC_TECHNOLOGY_A: nfc_a_print_tag(&tag->a);
            break;
        case NFC_TECHNOLOGY_B: nfc_b_print_tag(&tag->b);
            break;
        case NFC_TECHNOLOGY_F: nfc_f_print_tag(&tag->f);
            break;
        case NFC_TECHNOLOGY_V: nfc_v_print_tag(&tag->v);
            break;
        default:
            break;
    }
}

void nfc_a_print_tag(const nfc_a_tag_t* tag) {
    assert(tag);
    printf("<Tag%c id=[ ", 'A');
    for (size_t i = 0; i < tag->id.length; i += 1) {
        printf("%02x ", tag->id.uid[i]);
    }
    printf("] atqa=%02x%02x sak=%02x",
           tag->polling_response.raw[0], tag->polling_response.raw[1], tag->select_response);


    if (tag->ats.length > 0) {
        printf(" ats={");
        if (tag->ats.length >= 1) {
            printf(" fsc=(%u = %uB)",
                   tag->ats.max_frame_size, iso_dep_frame_size(tag->ats.max_frame_size));

            if (tag->ats.ta_present) {
                printf(" bitrates={ symmetric=%u ", tag->ats.ta_info.bitrates.same_constraint);
                printf("down+=[ ");
                for (uint8_t i = 0; i < 3; i += 1) {
                    if (tag->ats.ta_info.bitrates.down & (1 << i)) {
                        printf("%u ", nfc_bitrate_kbps(2 << i));
                    }
                }
                printf("] up+=[ ");
                for (uint8_t i = 0; i < 3; i += 1) {
                    if (tag->ats.ta_info.bitrates.up & (1 << i)) {
                        printf("%u ", nfc_bitrate_kbps(2 << i));
                    }
                }
                printf("] }");
            }

            if (tag->ats.tb_present) {
                printf(" sfgt=(%u = %ums) fwt=(%u = %ums)",
                       tag->ats.tb_info.startup_frame_guard_time,
                       nfc_time_index_ms(tag->ats.tb_info.startup_frame_guard_time),
                       tag->ats.tb_info.frame_waiting_time,
                       nfc_time_index_ms(tag->ats.tb_info.frame_waiting_time));
            }

            if (tag->ats.tc_present) {
                printf(" nad?=%u cid?=%u",
                tag->ats.tc_info.nad_supported, tag->ats.tc_info.cid_supported);
            }
        }
        size_t historical_length = nfc_a_ats_historical_length(&tag->ats);
        printf(" historical=[ ");
        for (size_t i = 0; i < historical_length; i += 1) {
            printf("%02x ", tag->historical[i]);
        }
        printf("] }");
    }
    printf(">");
}

void nfc_b_print_tag(const nfc_b_tag_t* tag) {
    assert(tag);
    printf("<Tag%c id=[ ", 'B');
    for (size_t i = 0; i < sizeof(nfc_b_id_t); i += 1) {
        printf("%02x ", tag->polling_response.id[i]);
    }
    printf("] application={ family=%02x crc=%02x%02x counts={total=%u family=%u} }",
               tag->polling_response.application_family,
               tag->polling_response.aid_crc[0], tag->polling_response.aid_crc[1],
               tag->polling_response.application_count_total,
               tag->polling_response.application_count_family);

        printf(" protocol={ bitrates={symmetric=%u down=[ ", tag->polling_response.bitrates.same_constraint);
        for (uint8_t i = 0; i < 3; i += 1) {
            if (tag->polling_response.bitrates.down & (1 << i)) {
                printf("%u ", nfc_bitrate_kbps(2 << i));
            }
        }
        printf("] up=[ ");
        for (uint8_t i = 0; i < 3; i += 1) {
            if (tag->polling_response.bitrates.up & (1 << i)) {
                printf("%u ", nfc_bitrate_kbps(2 << i));
            }
        }
        printf("] iso_dep=%u min_tr2=%u frame_size=%u cid?=%u nad?=%u app_data_standard=%u"
               " fwt=(%u ms) sfgt=(%u ms) }",
               tag->polling_response.iso_dep_supported,
               tag->polling_response.min_tr2,
               tag->polling_response.frame_size,
               tag->polling_response.cid_supported,
               tag->polling_response.nad_supported,
               tag->polling_response.application_data_standardized,
               nfc_time_index_ms(tag->polling_response.frame_waiting_time),
               nfc_time_index_ms(tag->polling_response.extension.startup_frame_guard_time));

        if (tag->attrib_response_length > 0) {
            assert(tag->attrib_response_length >= sizeof(nfc_b_attrib_response_t));
            printf(" attrib={cid=%u max_buflen_index=(%u bytes)}",
                   tag->attrib.cid,
                   tag->attrib.max_buffer_length_index);
            size_t higher_length = tag->attrib_response_length - sizeof(nfc_b_attrib_response_t);
            printf(" higher_layer=[ ");
            for (size_t i = 0; i < higher_length; i += 1) {
                printf("%02x ", tag->attrib.higher_layer[i]);
            }
            printf(" ]");
        }
        printf(">");
}

void nfc_f_print_tag(const nfc_f_tag_t* tag) {
    assert(tag);
    printf("<Tag%c id=[ ", 'F');
    for (size_t i = 0; i < sizeof(nfc_f_id_t); i += 1) {
        printf("%02x ", tag->id.raw[i]);
    }
    printf("] pmm=[ ");
    for (size_t i = 0; i < sizeof(nfc_f_pmm_t); i += 1) {
        printf("%02x ", tag->pmm.raw[i]);
    }
    printf("] system_code=%04x bitrates=[ ",
           tag->system_code);
    for (uint8_t i = 0; i < 2; i += 1) {
        if (tag->bitrates & (1 << i)) {
            printf("%u ", nfc_bitrate_kbps(2 << i));
        }
    }
    printf("]>");
}

void nfc_v_print_tag(const nfc_v_tag_t* tag) {
    assert(tag);
    printf("<Tag%c id=[ ", 'V');
    for (size_t i = 0; i < sizeof(nfc_v_id_t); i += 1) {
        printf("%02x ", tag->id[i]);
    }
    printf("] sfi=%02x bss_length_exp=%u>",
           tag->storage_format_identifier,
           tag->block_security_status_length_exponent);
}


void nfc_dep_print_atr_response(const nfc_dep_activation_response_t* atr, size_t length) {
    assert(atr);
    printf("<");
    if (length >= sizeof(nfc_dep_id_t)) {
        printf("id=[ ");
        for (size_t i = 0; i < sizeof(atr->id); i += 1) {
            printf("%02x ", atr->id[i]);
        }
        printf("]");
    }
    if (length >= sizeof(nfc_dep_activation_response_t)) {
        printf(" did=%02x additional_br={ down=%02x up=%02x } response_wait_max=(%" PRIu16 " ms)"
               " nad=%u general_bytes?=%u nfc_secure=%u payload_reduction=(%" PRIuSIZE " bytes)",
               atr->device_id,
               atr->supported_bitrates_rx,
               atr->supported_bitrates_tx,
               nfc_time_index_ms(atr->response_waiting_time),
               atr->nad_used,
               atr->general_bytes_available,
               atr->nfc_secure_supported,
               NFC_DEP_LENGTH_REDUCTION_IN_BYTES(atr->payload_reduction)
               );
        printf(" general_bytes=[ ");
        for (size_t i = 0; i < _nfc_dep_atr_response_general_length(length); i += 1) {
            printf("%02x ", atr->general_bytes[i]);
        }
        printf("]");
    }
    printf(">");
}

void nfc_dep_print_atr_request(const nfc_dep_activation_request_t* atr, size_t length) {
    assert(atr);
    printf("<");
    if (length >= sizeof(nfc_dep_id_t)) {
        printf("id=[ ");
        for (size_t i = 0; i < sizeof(atr->id); i += 1) {
            printf("%02x ", atr->id[i]);
        }
        printf("]");
    }
    if (length >= sizeof(nfc_dep_activation_request_t)) {
        printf(" did=%02x additional_br={ down=%02x up=%02x } response_wait_max=(%" PRIu16 " ms)"
               " nad=%u general_bytes?=%u nfc_secure=%u payload_reduction=(%" PRIuSIZE " bytes)",
               atr->device_id,
               atr->supported_bitrates_rx,
               atr->supported_bitrates_tx,
               0,
               atr->nad_used,
               atr->general_bytes_available,
               atr->nfc_secure_supported,
               NFC_DEP_LENGTH_REDUCTION_IN_BYTES(atr->payload_reduction)
               );
        printf(" general_bytes=[ ");
        for (size_t i = 0; i < _nfc_dep_atr_response_general_length(length); i += 1) {
            printf("%02x ", atr->general_bytes[i]);
        }
        printf("]");
    }
    printf(">");
}
