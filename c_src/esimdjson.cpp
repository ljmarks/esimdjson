#include "erl_nif.h"
#include "simdjson.h"

// See simdjson/simdjson.h:2050
char const *const error_msg[simdjson::NUM_ERROR_CODES]{
    /// No error
    [simdjson::SUCCESS] = "",
    /// This parser can't support a document that big
    [simdjson::CAPACITY] = "capacity",
    /// Error allocating memory, most likely out of memory
    [simdjson::MEMALLOC] = "memalloc",
    /// Something went wrong while writing to the tape (stage 2), this is a
    /// generic error
    [simdjson::TAPE_ERROR] = "tape_error",
    /// Your document exceeds the user-specified depth limitation
    [simdjson::DEPTH_ERROR] = "depth_error",
    /// Problem while parsing a string
    [simdjson::STRING_ERROR] = "string_error",
    /// Problem while parsing an atom starting with the letter 't'
    [simdjson::T_ATOM_ERROR] = "t_atom_error",
    /// Problem while parsing an atom starting with the letter 'f'
    [simdjson::F_ATOM_ERROR] = "f_atom_error",
    /// Problem while parsing an atom starting with the letter 'n'
    [simdjson::N_ATOM_ERROR] = "n_atom_error",
    /// Problem while parsing a number
    [simdjson::NUMBER_ERROR] = "number_error",
    /// The input is not valid UTF-8
    [simdjson::UTF8_ERROR] = "utf8_error",
    /// Unknown error, or uninitialized document
    [simdjson::UNINITIALIZED] = "uninitialized",
    /// No structural element found
    [simdjson::EMPTY] = "empty",
    /// Found unescaped characters in a string.
    [simdjson::UNESCAPED_CHARS] = "unescaped_chars",
    /// Missing quote at the end
    [simdjson::UNCLOSED_STRING] = "unclosed_string",
    /// Unsupported architecture
    [simdjson::UNSUPPORTED_ARCHITECTURE] = "unsupported_architecture",
    /// JSON element has a different type than user expected
    [simdjson::INCORRECT_TYPE] = "incorrect_type",
    /// JSON number does not fit in 64 bits
    [simdjson::NUMBER_OUT_OF_RANGE] = "number_out_of_range",
    /// JSON array index too large
    [simdjson::INDEX_OUT_OF_BOUNDS] = "index_out_of_bounds",
    /// JSON field not found in object
    [simdjson::NO_SUCH_FIELD] = "no_such_field",
    /// Error reading a file
    [simdjson::IO_ERROR] = "io_error",
    /// Invalid JSON pointer reference
    [simdjson::INVALID_JSON_POINTER] = "invalid_json_pointer",
    /// Invalid URI fragment
    [simdjson::INVALID_URI_FRAGMENT] = "invalid_uri_fragment",
    /// Indicative of a bug in simdjson
    [simdjson::UNEXPECTED_ERROR] = "unexpected_error",
    /// Parser is already in use.
    [simdjson::PARSER_IN_USE] = "parser_in_use",
};

ERL_NIF_TERM make_simdjson_error(ErlNifEnv *env,
                                 const simdjson::error_code error);
ERL_NIF_TERM make_atom(ErlNifEnv *env, const char *atom);
ERL_NIF_TERM make_ok_result(ErlNifEnv *env, const ERL_NIF_TERM result);
ERL_NIF_TERM make_error(ErlNifEnv *env, const ERL_NIF_TERM reason);
static ERL_NIF_TERM nif_parse(ErlNifEnv *env, const int argc,
                              const ERL_NIF_TERM argv[]);
static ERL_NIF_TERM nif_load(ErlNifEnv *env, const int argc,
                             const ERL_NIF_TERM argv[]);
static ERL_NIF_TERM nif_new(ErlNifEnv *env, const int argc,
                            const ERL_NIF_TERM argv[]);
int load(ErlNifEnv *env, void **priv_data, const ERL_NIF_TERM load_info);
int make_term_from_dom(ErlNifEnv *env, const simdjson::dom::element element,
                       ERL_NIF_TERM *term);
void dom_parser_dtor(ErlNifEnv *env, void *obj);

int load(ErlNifEnv *env, void **priv_data, const ERL_NIF_TERM load_info) {
  ErlNifResourceFlags flags =
      ErlNifResourceFlags(ERL_NIF_RT_CREATE | ERL_NIF_RT_TAKEOVER);
  ErlNifResourceType *res_type = enif_open_resource_type(
      env, nullptr, "esimdjson_dom_parser", dom_parser_dtor, flags, nullptr);
  if (!res_type)
    return -1;
  *priv_data = (void *)res_type;
  return 0;
}

ERL_NIF_TERM make_atom(ErlNifEnv *env, const char *atom) {
  ERL_NIF_TERM ret;

  if (!enif_make_existing_atom(env, atom, &ret, ERL_NIF_LATIN1)) {
    return enif_make_atom(env, atom);
  }

  return ret;
}

ERL_NIF_TERM make_ok_result(ErlNifEnv *env, const ERL_NIF_TERM result) {
  return enif_make_tuple2(env, make_atom(env, "ok"), result);
}

ERL_NIF_TERM make_error(ErlNifEnv *env, const ERL_NIF_TERM reason) {
  return enif_make_tuple2(env, make_atom(env, "error"), reason);
}

ERL_NIF_TERM make_simdjson_error(ErlNifEnv *env,
                                 const simdjson::error_code error) {
  ERL_NIF_TERM reason_atom = make_atom(env, error_msg[error]);
  std::string error_str = simdjson::error_message(error);
  ERL_NIF_TERM reason_str =
      enif_make_string(env, error_str.data(), ERL_NIF_LATIN1);
  ERL_NIF_TERM reason = enif_make_tuple2(env, reason_atom, reason_str);

  return make_error(env, reason);
}

static ERL_NIF_TERM nif_new(ErlNifEnv *env, const int argc,
                            const ERL_NIF_TERM argv[]) {
  /*TODO:
   * - max_capacity option
   * (https://github.com/simdjson/simdjson/blob/master/doc/performance.md#reusing-the-parser-for-maximum-efficiency)
   * - fixed_capacity option (ditto)
   */
  ErlNifResourceType *res_type = (ErlNifResourceType *)enif_priv_data(env);

  void *parser_res =
      enif_alloc_resource(res_type, sizeof(simdjson::dom::parser));

  // "placement new"
  new (parser_res) simdjson::dom::parser();

  ERL_NIF_TERM res_term = enif_make_resource(env, parser_res);
  enif_release_resource(parser_res);

  return make_ok_result(env, res_term);
}

static ERL_NIF_TERM nif_load(ErlNifEnv *env, const int argc,
                             const ERL_NIF_TERM argv[]) {
  if (argc != 2)
    return enif_make_badarg(env);

  ErlNifResourceType *res_type = (ErlNifResourceType *)enif_priv_data(env);
  simdjson::dom::parser *pparser;
  if (!enif_get_resource(env, argv[0], res_type, (void **)&pparser))
    return enif_make_badarg(env);

  unsigned int path_size;
  if (!enif_get_list_length(env, argv[1], &path_size))
    return enif_make_badarg(env);
  char path[path_size + 1];
  if (!enif_get_string(env, argv[1], path, path_size + 1, ERL_NIF_LATIN1))
    return enif_make_badarg(env);

  simdjson::dom::element element;

  auto error = pparser->load(path).get(element);
  if (error) {
    return make_simdjson_error(env, error);
  }

  ERL_NIF_TERM result;
  make_term_from_dom(env, element, &result);

  return make_ok_result(env, result);
}

static ERL_NIF_TERM nif_parse(ErlNifEnv *env, const int argc,
                              const ERL_NIF_TERM argv[]) {
  if (argc != 2)
    return enif_make_badarg(env);

  ErlNifResourceType *res_type = (ErlNifResourceType *)enif_priv_data(env);
  simdjson::dom::parser *pparser;
  if (!enif_get_resource(env, argv[0], res_type, (void **)&pparser))
    return enif_make_badarg(env);

  ErlNifBinary bin;
  if (!enif_inspect_binary(env, argv[1], &bin))
    return enif_make_badarg(env);

  simdjson::dom::element element;
  auto error = pparser->parse((char *)bin.data, bin.size).get(element);
  if (error)
    return make_simdjson_error(env, error);

  ERL_NIF_TERM result;
  make_term_from_dom(env, element, &result);

  return make_ok_result(env, result);
}

int make_term_from_dom(ErlNifEnv *env, const simdjson::dom::element element,
                       ERL_NIF_TERM *term) {
  switch (element.type()) {
  case simdjson::dom::element_type::INT64:
    *term = enif_make_int64(env, int64_t(element));
    break;
  case simdjson::dom::element_type::UINT64:
    *term = enif_make_uint64(env, uint64_t(element));
    break;
  case simdjson::dom::element_type::DOUBLE:
    *term = enif_make_double(env, double(element));
    break;
  case simdjson::dom::element_type::STRING: {
    std::string_view str = std::string_view(element);
    char *str_bin = (char *)enif_make_new_binary(env, str.size(), term);
    str.copy(str_bin, str.size());
  } break;
  case simdjson::dom::element_type::OBJECT: {
    simdjson::dom::object obj = simdjson::dom::object(element);
    ERL_NIF_TERM keys[obj.size()];
    ERL_NIF_TERM values[obj.size()];

    size_t i = 0;
    for (auto [key, value] : obj) {
      ERL_NIF_TERM k;
      char *k_bin = (char *)enif_make_new_binary(env, key.size(), &k);
      key.copy(k_bin, key.size());

      ERL_NIF_TERM v;
      make_term_from_dom(env, value, &v);
      keys[i] = k;
      values[i] = v;
      i++;
    }
    enif_make_map_from_arrays(env, keys, values, obj.size(), term);

  } break;
  case simdjson::dom::element_type::ARRAY: {
    ERL_NIF_TERM cons = enif_make_list(env, 0);

    simdjson::dom::array array = simdjson::dom::array(element);

    for (size_t i = array.size(); i > 0; i--) {
      simdjson::dom::element e = array.at(i - 1);
      ERL_NIF_TERM car;
      make_term_from_dom(env, e, &car);
      cons = enif_make_list_cell(env, car, cons);
    }
    *term = cons;

  } break;
  default:
    break;
  }

  return 0;
}

void dom_parser_dtor(ErlNifEnv *env, void *obj) {
  // Memory deallocation is done by Erlang GC since we released the resource
  // with `enif_release_resource`, so we only need to do object destruction.
  simdjson::dom::parser *pparser = (simdjson::dom::parser *)obj;
  pparser->~parser();
}

static ErlNifFunc nif_funcs[] = {
    {"parse", 2, nif_parse, ERL_NIF_DIRTY_JOB_CPU_BOUND},
    {"load", 2, nif_load, ERL_NIF_DIRTY_JOB_CPU_BOUND},
    {"new", 1, nif_new},
};

ERL_NIF_INIT(esimdjson, nif_funcs, load, nullptr, nullptr, nullptr);
