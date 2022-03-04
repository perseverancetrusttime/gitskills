/* Automatically generated nanopb header */
/* Generated by nanopb-0.3.9 at Mon May 18 15:15:42 2020. */

#ifndef PB_GOOGLE_PROTOBUF_STRUCT_PB_H_INCLUDED
#define PB_GOOGLE_PROTOBUF_STRUCT_PB_H_INCLUDED
#include <pb.h>

/* @@protoc_insertion_point(includes) */
#if PB_PROTO_HEADER_VERSION != 30
#error Regenerate this file with the current version of nanopb generator.
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Enum definitions */
typedef enum _google_protobuf_NullValue {
    google_protobuf_NullValue_NULL_VALUE = 0
} google_protobuf_NullValue;
#define _google_protobuf_NullValue_MIN google_protobuf_NullValue_NULL_VALUE
#define _google_protobuf_NullValue_MAX google_protobuf_NullValue_NULL_VALUE
#define _google_protobuf_NullValue_ARRAYSIZE ((google_protobuf_NullValue)(google_protobuf_NullValue_NULL_VALUE+1))

/* Struct definitions */
typedef struct _google_protobuf_ListValue {
    pb_callback_t values;
/* @@protoc_insertion_point(struct:google_protobuf_ListValue) */
} google_protobuf_ListValue;

typedef struct _google_protobuf_Struct {
    pb_callback_t fields;
/* @@protoc_insertion_point(struct:google_protobuf_Struct) */
} google_protobuf_Struct;

typedef struct _google_protobuf_Value {
    pb_size_t which_kind;
    union {
        google_protobuf_NullValue null_value;
        double number_value;
        char string_value[100];
        bool bool_value;
    } kind;
/* @@protoc_insertion_point(struct:google_protobuf_Value) */
} google_protobuf_Value;

typedef struct _google_protobuf_Struct_FieldsEntry {
    pb_callback_t key;
    google_protobuf_Value value;
/* @@protoc_insertion_point(struct:google_protobuf_Struct_FieldsEntry) */
} google_protobuf_Struct_FieldsEntry;

/* Default values for struct fields */

/* Initializer values for message structs */
#define google_protobuf_Struct_init_default      {{{NULL}, NULL}}
#define google_protobuf_Struct_FieldsEntry_init_default {{{NULL}, NULL}, google_protobuf_Value_init_default}
#define google_protobuf_Value_init_default       {0, {(google_protobuf_NullValue)0}}
#define google_protobuf_ListValue_init_default   {{{NULL}, NULL}}
#define google_protobuf_Struct_init_zero         {{{NULL}, NULL}}
#define google_protobuf_Struct_FieldsEntry_init_zero {{{NULL}, NULL}, google_protobuf_Value_init_zero}
#define google_protobuf_Value_init_zero          {0, {(google_protobuf_NullValue)0}}
#define google_protobuf_ListValue_init_zero      {{{NULL}, NULL}}

/* Field tags (for use in manual encoding/decoding) */
#define google_protobuf_ListValue_values_tag     1
#define google_protobuf_Struct_fields_tag        1
#define google_protobuf_Value_null_value_tag     1
#define google_protobuf_Value_number_value_tag   2
#define google_protobuf_Value_string_value_tag   3
#define google_protobuf_Value_bool_value_tag     4
#define google_protobuf_Struct_FieldsEntry_key_tag 1
#define google_protobuf_Struct_FieldsEntry_value_tag 2

/* Struct field encoding specification for nanopb */
extern const pb_field_t google_protobuf_Struct_fields[2];
extern const pb_field_t google_protobuf_Struct_FieldsEntry_fields[3];
extern const pb_field_t google_protobuf_Value_fields[5];
extern const pb_field_t google_protobuf_ListValue_fields[2];

/* Maximum encoded size of messages (where known) */
/* google_protobuf_Struct_size depends on runtime parameters */
/* google_protobuf_Struct_FieldsEntry_size depends on runtime parameters */
#define google_protobuf_Value_size               102
/* google_protobuf_ListValue_size depends on runtime parameters */

/* Message IDs (where set with "msgid" option) */
#ifdef PB_MSGID

#define STRUCT_MESSAGES \


#endif

#ifdef __cplusplus
} /* extern "C" */
#endif
/* @@protoc_insertion_point(eof) */

#endif
