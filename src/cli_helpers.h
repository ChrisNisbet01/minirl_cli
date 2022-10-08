#ifndef CLI_HELPERS_H
#define CLI_HELPERS_H

#include <stdbool.h>

#if 0
#define LOGICAL_AND 1
#define LOGICAL_OR  2
#define LOGICAL_END 4
#define BRACKET_CL  8

struct config_node;
struct schema_node;

struct cli_commands {
	char  id;
	char* cmd;
	int   priority;
	int   op_code;
	int   result;
	int   count;
	struct cli_commands* next;
	char*  eval_line;
};
#endif

struct cli_split {
	char **words;
	char *partial;
	char *partial_raw;
};

#if 0
/* dump cli_commands structure to stderr, used for debugging */
void cli_dump_command_info(struct cli_commands *commands);

/* populate cli_commands struct from commandline */
void cli_parse_cmdline(struct cli_commands *commands, char* cmdline);

/* return next command to execute */
struct cli_commands* cli_get_next_cmd(struct cli_commands *commands, char *last_id);

/* return true if still possible to evaluate */
bool cli_evaluate_expression(struct cli_commands *commands, bool *result, char id);

/* return result of expression with all unknowns false */
bool cli_evaluate(char *command);

/* free memory used by cli_commands struct */
void cli_commands_cleanup(struct cli_commands *commands);

/* Space-separated config path */
char *cli_config_path(struct config_node *node);

/* Reset node to default values */
void cli_config_revert(struct config_node *node);

bool cli_config_node_is_field(struct config_node *node);

bool cli_config_array_contains_string(struct config_node *array,
	const char *str);

/* Returns true if node has ancestor in the parent chain up to the
 * config root. */
bool cli_config_node_is_ancestor(struct config_node *node,
	struct config_node *ancestor);

/* Get index of element in its parent array */
int cli_config_array_element_index(struct config_node *child);

/* Returns true if node is a array item or additional property */
bool cli_config_node_is_dynamic(struct config_node *node);

/* Get the keys of the object node */
char **cli_config_node_keys(struct config_node *node, bool fixed, bool dynamic);

/* Check that the string is a valid key under the parent node */
bool cli_config_key_validate(struct config_node *parent, const char *key);

/* Get the string-representations of the values that the node may be set to */
char **cli_config_node_values(struct config_node *node);

/* Returns true if the node may be set with the value string. */
bool cli_config_node_validate(struct config_node *node, const char *val,
	bool strict_ref);

/* Get the string representation of the node value */
char *cli_config_node_get(struct config_node *node, bool redacted, bool escaped);

/* Set the node value with a string representation of the value */
void cli_config_node_set(struct config_node *node, const char *val);

/* Like libconfig's config_lookup() but without flagging errors if the lookup fails */
struct config_node *cli_config_lookup(struct config_node *parent, const char *path);

/* Return true if the node has dynamically addable fields (strings/ints/etc) */
bool cli_config_container_contains_fields(struct config_node *parent);

/* Get the string-representations of the values that dynamically added
 * child elements may be set to */
char **cli_config_container_values(struct config_node *parent);

/* Returns true if a dynamically added child of the parent may be set with
 * the value string */
bool cli_config_container_validate(struct config_node *parent, const char *val);

/* Generate the CLI commands required to create the configuration */
char *cli_config_commands(struct config_node *node);
#endif

void cli_split_line(const char *s, bool partial, struct cli_split *split);
void cli_split_free(struct cli_split *split);

#if 0
void cli_print_config_help(struct config_node *node, bool arg_format);
void cli_printf(const char *fmt, ...);
void cli_print(const char *msg);
void cli_print_separator(char c, int n);
void cli_print_table(
	char **col1, int maxlen1,
	char **col2, int maxlen2,
	char **col3, int maxlen3,
	bool heading);
#endif

#endif
