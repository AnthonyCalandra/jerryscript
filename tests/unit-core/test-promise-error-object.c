/* Copyright JS Foundation and other contributors, http://js.foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "jerryscript.h"
#include "jerryscript-port.h"
#include "jerryscript-port-default.h"
#include "test-common.h"


const char *test_source = (
                           "var p = create_promise();"
                           "p.then(function(x) { "
                           "  assert(x==='resolved'); "
                           "}); "
                           "p.catch(function(x) { "
                           "  assert(typeof x==='object'); "
                           "}); "
                           );

static int count_in_assert = 0;
static jerry_value_t my_promise;
const jerry_error_t resolve_error = JERRY_ERROR_COMMON;
const jerry_char_t resolve_msg[] = "resolved";
const jerry_char_t error_msg[] = "rejected";

static jerry_value_t
create_promise_handler (const jerry_value_t func_obj_val, /**< function object */
                         const jerry_value_t this_val, /**< this value */
                         const jerry_value_t args_p[], /**< arguments list */
                         const jerry_length_t args_cnt) /**< arguments length */
{
  JERRY_UNUSED (func_obj_val);
  JERRY_UNUSED (this_val);
  JERRY_UNUSED (args_p);
  JERRY_UNUSED (args_cnt);

  my_promise = jerry_create_promise ();

  jerry_value_t error_reject = jerry_create_error (resolve_error, error_msg);
  jerry_value_t result = jerry_resolve_or_reject_promise (my_promise, error_reject, false);
  TEST_ASSERT (!jerry_value_is_error (result));

  jerry_release_value (result);
  jerry_release_value (error_reject);
  return my_promise;
} /* create_promise_handler */

static jerry_value_t
assert_handler (const jerry_value_t func_obj_val, /**< function object */
                const jerry_value_t this_val, /**< this arg */
                const jerry_value_t args_p[], /**< function arguments */
                const jerry_length_t args_cnt) /**< number of function arguments */
{
  JERRY_UNUSED (func_obj_val);
  JERRY_UNUSED (this_val);

  count_in_assert++;

  if (args_cnt == 1
      && jerry_value_is_boolean (args_p[0])
      && jerry_get_boolean_value (args_p[0]))
  {
    return jerry_create_boolean (true);
  }
  else
  {
    TEST_ASSERT (false);
  }
} /* assert_handler */

/**
 * Register a JavaScript function in the global object.
 */
static void
register_js_function (const char *name_p, /**< name of the function */
                      jerry_external_handler_t handler_p) /**< function callback */
{
  jerry_value_t global_obj_val = jerry_get_global_object ();

  jerry_value_t function_val = jerry_create_external_function (handler_p);
  jerry_value_t function_name_val = jerry_create_string ((const jerry_char_t *) name_p);
  jerry_value_t result_val = jerry_set_property (global_obj_val, function_name_val, function_val);

  jerry_release_value (function_name_val);
  jerry_release_value (function_val);
  jerry_release_value (global_obj_val);

  jerry_release_value (result_val);
} /* register_js_function */

int
main (void)
{
  jerry_init (JERRY_INIT_EMPTY);

  if (!jerry_is_feature_enabled (JERRY_FEATURE_PROMISE))
  {
    jerry_port_log (JERRY_LOG_LEVEL_ERROR, "Promise is disabled!\n");
    jerry_cleanup ();
    return 0;
  }

  register_js_function ("create_promise", create_promise_handler);
  register_js_function ("assert", assert_handler);

  jerry_value_t parsed_code_val = jerry_parse (NULL,
                                               0,
                                               (jerry_char_t *) test_source,
                                               strlen (test_source),
                                               JERRY_PARSE_NO_OPTS);
  TEST_ASSERT (!jerry_value_is_error (parsed_code_val));

  jerry_value_t res = jerry_run (parsed_code_val);
  TEST_ASSERT (!jerry_value_is_error (res));

  jerry_release_value (res);
  jerry_release_value (parsed_code_val);

  /* Test jerry_create_promise and jerry_value_is_promise. */
  TEST_ASSERT (jerry_value_is_promise (my_promise));

  TEST_ASSERT (count_in_assert == 0);

  /* Run the jobqueue. */
  res = jerry_run_all_enqueued_jobs ();

  TEST_ASSERT (!jerry_value_is_error (res));
  TEST_ASSERT (count_in_assert == 1);

  jerry_release_value (my_promise);

  jerry_cleanup ();

  return 0;
} /* main */
