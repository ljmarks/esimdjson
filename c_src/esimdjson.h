#include "erl_nif.h"
#include "simdjson.h"

static ERL_NIF_TERM atom_ok;
static ERL_NIF_TERM atom_error;
static ERL_NIF_TERM atom_null;
static ERL_NIF_TERM atom_true;
static ERL_NIF_TERM atom_false;

struct error_txt {
  simdjson::error_code code;
  const char *txt;
};

/// String literals used for making atoms for error tuples
const error_txt error_code_txt[]{
    /// No error
    {simdjson::SUCCESS, ""},
    /// This parser can't support a document that big
    {simdjson::CAPACITY, "capacity"},
    /// Error allocating memory, most likely out of memory
    {simdjson::MEMALLOC, "memalloc"},
    /// Something went wrong while writing to the tape (stage 2), this is a
    /// generic error
    {simdjson::TAPE_ERROR, "tape_error"},
    /// Your document exceeds the user-specified depth limitation
    {simdjson::DEPTH_ERROR, "depth_error"},
    /// Problem while parsing a string
    {simdjson::STRING_ERROR, "string_error"},
    /// Problem while parsing an atom starting with the letter 't'
    {simdjson::T_ATOM_ERROR, "t_atom_error"},
    /// Problem while parsing an atom starting with the letter 'f'
    {simdjson::F_ATOM_ERROR, "f_atom_error"},
    /// Problem while parsing an atom starting with the letter 'n'
    {simdjson::N_ATOM_ERROR, "n_atom_error"},
    /// Problem while parsing a number
    {simdjson::NUMBER_ERROR, "number_error"},
    /// The input is not valid UTF-8
    {simdjson::UTF8_ERROR, "utf8_error"},
    /// Unknown error, or uninitialized document
    {simdjson::UNINITIALIZED, "uninitialized"},
    /// No structural element found
    {simdjson::EMPTY, "empty"},
    /// Found unescaped characters in a string.
    {simdjson::UNESCAPED_CHARS, "unescaped_chars"},
    /// Missing quote at the end
    {simdjson::UNCLOSED_STRING, "unclosed_string"},
    /// Unsupported architecture
    {simdjson::UNSUPPORTED_ARCHITECTURE, "unsupported_architecture"},
    /// JSON element has a different type than user expected
    {simdjson::INCORRECT_TYPE, "incorrect_type"},
    /// JSON number does not fit in 64 bits
    {simdjson::NUMBER_OUT_OF_RANGE, "number_out_of_range"},
    /// JSON array index too large
    {simdjson::INDEX_OUT_OF_BOUNDS, "index_out_of_bounds"},
    /// JSON field not found in object
    {simdjson::NO_SUCH_FIELD, "no_such_field"},
    /// Error reading a file
    {simdjson::IO_ERROR, "io_error"},
    /// Invalid JSON pointer reference
    {simdjson::INVALID_JSON_POINTER, "invalid_json_pointer"},
    /// Invalid URI fragment
    {simdjson::INVALID_URI_FRAGMENT, "invalid_uri_fragment"},
    /// Indicative of a bug in simdjson
    {simdjson::UNEXPECTED_ERROR, "unexpected_error"},
    /// Parser is already in use.
    {simdjson::PARSER_IN_USE, "parser_in_use"},
};

/// NIF interface declarations
static int load(ErlNifEnv *env, void **priv_data, const ERL_NIF_TERM load_info);

/// Actual NIF declarations
static ERL_NIF_TERM nif_parse(ErlNifEnv *env, const int argc,
                              const ERL_NIF_TERM argv[]);
static ERL_NIF_TERM nif_load(ErlNifEnv *env, const int argc,
                             const ERL_NIF_TERM argv[]);
static ERL_NIF_TERM nif_new(ErlNifEnv *env, const int argc,
                            const ERL_NIF_TERM argv[]);

ERL_NIF_TERM make_simdjson_error(ErlNifEnv *env,
                                 const simdjson::error_code error);
ERL_NIF_TERM make_atom(ErlNifEnv *env, const char *atom);
ERL_NIF_TERM make_ok_result(ErlNifEnv *env, const ERL_NIF_TERM result);
ERL_NIF_TERM make_error(ErlNifEnv *env, const ERL_NIF_TERM reason);
int make_term_from_dom(ErlNifEnv *env, const simdjson::dom::element element,
                       ERL_NIF_TERM *term);
void dom_parser_dtor(ErlNifEnv *env, void *obj);
