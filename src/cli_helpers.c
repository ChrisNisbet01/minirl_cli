#include "cli_helpers.h"

#if 0
#include <libconfig/config_internal.h>
#include <libconfig/schema.h>
#include <libconfig/obfuscate.h>
#include <libconfig/config.h>
#include <openssl/pem.h>
#include "cmd.h"
#endif

#include <ctype.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <fnmatch.h>
#include <sl.h>
#include <util.h>


#if 0
void cli_dump_command_info(struct cli_commands *commands)
{
	struct cli_commands *current = commands;

	while ( current->next != NULL ) {

		fprintf(stderr, "id:%c, priority:%d, op_code:%d, command:%s, result:%d\n",
			current->id, current->priority, current->op_code, current->cmd, current->result);

		current = current->next;
	}
	fprintf(stderr, "id:%c, priority:%d, op_code:%d, command:%s, result:%d\n",
		current->id, current->priority, current->op_code, current->cmd, current->result);

	fprintf(stderr, "count:%d, eval_line:%s\n", commands->count, commands->eval_line);
}

void cli_parse_cmdline(struct cli_commands *commands, char* cmdline)
{
	char* start_cmd = NULL;
	char* end_cmd = NULL;

	bool logic = true;
	char id = 'A';
	int  idx = 0;
	int  eval_idx = 0;
	int  priority = 0;

	struct cli_commands* current = commands;
	struct cli_commands* previous = NULL;

	commands->eval_line = (char*) malloc(strlen(cmdline)+1);
	memset(commands->eval_line, 0, sizeof(commands->eval_line));

	for (char *p = cmdline; *p != '\0'; p++) {

		bool special = false;

		switch( *p ) {
		case '(':
			if ( p != cmdline && *(p-1) != '\\' ) {
				special = true;
				commands->eval_line[eval_idx] = *p;
				eval_idx++;
				priority++;
				idx++;
			}
			break;
		case ')':
			if ( p != cmdline && *(p-1) != '\\' ) {
				special = true;
				commands->eval_line[eval_idx] = *p;
				eval_idx++;
				priority--;
				idx++;
			}
			break;
		case '&':
			if ( ( p != cmdline && *(p-1) == '&' ) || *(p+1) == '&' ) {
				special = true;
				commands->eval_line[eval_idx] = *p;
				eval_idx++;
				idx++;
			}
			break;
		case '|':
			if ( ( p != cmdline && *(p-1) == '|' ) || *(p+1) == '|' ) {
				special = true;
				commands->eval_line[eval_idx] = *p;
				eval_idx++;
				idx++;
			}
			break;
		case ';':
			special = true;
			logic = false;
			current->op_code = 0;
			idx++;
			break;
		}

		if (special && start_cmd != NULL && end_cmd != NULL ) {

			// end of command
			current->cmd = strndup(start_cmd, end_cmd-start_cmd+1);
			start_cmd = NULL;
			end_cmd = NULL;
			id++;

			if ( *p == ')' )
				current->op_code = BRACKET_CL;
			else if ( *p == '&' )
				current->op_code = LOGICAL_AND;
			else if ( *p == '|' ) {
				current->op_code = LOGICAL_OR;
			}

			current->next = (struct cli_commands*) malloc(sizeof(struct cli_commands));
			previous = current;
			current = current->next;
			current->id = '\0';
			current->result = -1;
			current->priority = 0;
			current->op_code = 0;
			current->next = 0;
		}
		else if ( !special ) {
			if ( start_cmd == NULL && *p == ' ' ) {
				// remove leading whitespace
			}
			else if ( start_cmd == NULL ) {

				if ( commands->count == 10 ) {
					commands->count++;
					commands->eval_line[eval_idx] = '\0';
					return;
				}

				// start of command
				start_cmd = p;
				current->id = id;
				current->result = -1;
				current->priority = priority;
				current->op_code = 0;
				current->next = 0;
				commands->count++;
				commands->eval_line[eval_idx] = id;
				eval_idx++;
			}
			else {
				end_cmd = p;
			}
		}
	}
	if ( start_cmd != NULL && end_cmd != NULL ) {
		current->id = id;
		current->result = -1;
		current->priority = priority;
		current->op_code = 0;
		current->cmd = (char*)malloc(end_cmd-start_cmd+1);
		current->cmd = strndup(start_cmd, end_cmd-start_cmd+1);
	}
	else {
		if (current->next) {
			free(current->next);
			current->next = NULL;
		}
		if (previous && current) {
			free(current);
			previous->next = NULL;
		}
	}
	if ( logic && !current->op_code)
		current->op_code = LOGICAL_END;

	commands->eval_line[eval_idx] = '\0';
}

struct cli_commands* cli_get_next_cmd(struct cli_commands *commands, char *last_id)
{
	int max_priority = 0;
	struct cli_commands* selected = NULL;
	struct cli_commands* current = commands;

	// get highest priority with result -1
	while ( current && current->next != NULL ) {
		if ( current->result == -1 && current->priority > max_priority )
			max_priority = current->priority;
		current = current->next;
	}
	if ( current && current->result == -1 && current->priority > max_priority )
		max_priority = current->priority;

	// find 1st command with max_priority & result -1
	current = commands;
	while ( current && current->next != NULL ) {
		if ( current->priority == max_priority && current->result == -1 ) {
			selected = current;
			break;
		}
		current = current->next;
	}
	if ( current && current->priority == max_priority && current->result == -1 )
		selected = current;

	return selected;
}

bool cli_evaluate_expression(struct cli_commands *commands, bool *result, char id)
{
	bool ok = false;

	struct cli_commands* current = commands;
	while ( current && current->next != NULL ) {
		if ( current->id == id ) {
			char result = '0' + current->result;
			for (char *p = commands->eval_line; *p != '\0'; p++) {
				if ( *p == current->id ) {
					*p = result;
					ok = true;
					break;
				}
			}
			break;
		}
		current = current->next;
	}
	if ( current && current->id == id && !ok ) {
		char result = '0' + current->result;
		for (char *p = commands->eval_line; *p != '\0'; p++) {
			if ( *p == current->id ) {
				*p = result;
				ok = true;
				break;
			}
		}
	}
	if ( current && !current->result &&
		current->next && current->next->priority == current->priority &&
		current->op_code == LOGICAL_AND  && current->priority > 0 ) {
		// no need to run next
		current->next->result = 0;
		for (char *p = commands->eval_line; *p != '\0'; p++) {
			if ( *p == current->next->id ) {
				*p = '0';
			}
		}
	}
	if ( current && current->result &&
		current->next && current->next->priority == current->priority &&
		current->op_code == LOGICAL_OR  && current->priority > 0 ) {
		// no need to run next
		current->next->result = 0;
		for (char *p = commands->eval_line; *p != '\0'; p++) {
			if ( *p == current->next->id ) {
				*p = '1';
			}
		}
	}

	// determine resolvable
	int idx = 0;
	char partial[strlen(commands->eval_line)+1];
	strcpy(partial, commands->eval_line);
	for (char *p = commands->eval_line; *p != '\0'; p++) {
		if ( isalpha(*p) )
			partial[idx] = '0';
		idx++;
	}
	partial[idx] = '\0';
	*result = cli_evaluate(partial);

	ok = *result;

	if ( !ok ) {
		// bail if we cannot succeed
		idx = 0;
		strcpy(partial, commands->eval_line);
		for (char *p = commands->eval_line; *p != '\0'; p++) {
			if ( isalpha(*p) )
				partial[idx] = '1';
			idx++;
		}
		partial[idx] = '\0';
		ok = !cli_evaluate(partial);
	}
	return ok;
}

bool cli_evaluate(char *command)
{
	int ret = 1;
	char expr_line[strlen(command)+32];

	sprintf(expr_line,"echo $\(\( %s )) 2>/dev/null", command);
	char line[32];
	memset(line,0,sizeof(line));

	FILE *fp = popen(expr_line, "r");

	if (fp == NULL)
		fprintf(stderr,"Failed to run:%s\n", expr_line);
	else {
		fgets(line, sizeof(line), fp);
		ret = atoi(line);
		pclose(fp);
	}
	return ret;
}

void cli_commands_cleanup(struct cli_commands *commands)
{
	struct cli_commands *current = commands;

	while ( current->next != NULL ) {

		struct cli_commands * next = current->next;

		free(current->cmd);
		if (current != commands)
			free(current);

		current = next;
	}
	free(commands->eval_line);
}
#endif

/* FIXME: UTF-8 */
void cli_split_line(const char *s, bool partial, struct cli_split *split)
{
	const char *from;
	const char *last;
	char *tmp, *to;
	bool quote;
	bool escape;

	tmp = malloc(strlen(s));

	split->words = sl_new(NULL);
	from = s;
	do {
		while (isspace(*from))
			from++;

		last = from;
		if (!*from)
			break;

		quote = false;
		escape = false;
		to = tmp;
		while (*from) {
			if (escape) {
				escape = false;
				*to = *from;
				to++;
			} else if (*from == '\\') {
				escape = true;
			} else if (*from == '"') {
				quote = !quote;
			} else if (!quote && isspace(*from)) {
				break;
			} else {
				*to = *from;
				to++;
			}

			from++;
		}

		split->words = sl_append_len(split->words, tmp, to - tmp);
	} while (*from);

	if (partial && *last) {
		split->partial = sl_pop(split->words);
		split->partial_raw = strdup(last);
	} else {
		split->partial = NULL;
		split->partial_raw = NULL;
	}

	free(tmp);
}

void cli_split_free(struct cli_split *split)
{
	sl_free(split->words);
	free(split->partial);
	free(split->partial_raw);
}

#if 0
char *cli_config_path(struct config_node *node)
{
	char **l;
	char *s;

	l = config_path_sl(node);
	if (!l)
		return NULL;

	s = sl_join(l, " ");
	sl_free(l);

	return s;
}

void cli_config_revert(struct config_node *node)
{
	struct schema_node *schema;
	struct config_node *defaults;
	struct config_node *root;
	struct config_node *parent;
	char **l, **s;

	parent = config_node_parent(node);
	if (config_is_root(parent)) {
		/* Can't replace the root. Instead, replace all top level objects
		 * which are practically guaranteed to not be dynamic children */
		root = node;
		l = config_keys(root);
		for (s = l; *s; s++) {
			/* Don't revert top level "schema" object. It contains the
			 * "version" of the config which needs to remain. Reverting it
			 * to empty causes the config to be migrated to the current
			 * schema version when it is loaded. */
			if (strcmp(*s, "schema") == 0)
				continue;
			node = config_lookup_single(root, *s);
			schema = config_node_schema(node);
			defaults = config_node_new(schema);
			config_object_replace_internal(node, defaults);
		}
		sl_free(l);
	} else {
		schema = config_node_schema(node);
		defaults = config_node_new(schema);
		config_object_replace_internal(node, defaults);
	}
}

bool cli_config_node_is_ancestor(struct config_node *node,
	struct config_node *ancestor)
{
	while (node && node != ancestor)
		node = config_node_parent(node);
	return node && node == ancestor;
}

bool cli_config_node_is_dynamic(struct config_node *node)
{
	struct config_node *parent = config_node_parent(node);

	/* All array elements are dynamic */
	if (config_is_array(parent))
		return true;

	if (!config_is_object(parent))
		return false;
	/* Dynamic object children use the schema from the parent's additionalProperties */
	return config_node_schema(node) == config_object_additional_schema(parent);
}

bool cli_config_node_is_field(struct config_node *node)
{
	struct schema_node *schema;

	schema = config_node_schema(node);
	if (!schema)
		return false;

	switch (schema_node_type(schema)) {
	case SCHEMA_TYPE_STRING:
	case SCHEMA_TYPE_INT:
	case SCHEMA_TYPE_NUMBER:
	case SCHEMA_TYPE_BOOL:
		return true;
		break;
	default:
		return false;
	}
}

bool cli_config_array_contains_string(struct config_node *array,
	const char *str)
{
	size_t i, len;

	len = config_array_count(array);
	for (i = 0; i < len; i++) {
		if (strcmp(str, config_array_get_string(array, i)) == 0)
			return true;
	}
	return false;
}

int cli_config_array_element_index(struct config_node *child)
{
	struct config_node *array = config_node_parent(child);
	int i, len;

	if (!config_is_array(array))
		return -1;

	len = config_array_count(array);
	for (i = 0; i < len; i++) {
		if (child == config_array_get(array, i))
			return i;
	}
	return -1;
}

char **cli_config_node_keys(struct config_node *node, bool fixed, bool dynamic)
{
	if (config_is_object(node)) {
		return config_object_keys(node, fixed, dynamic, true);
	} else if (config_is_array(node)) {
		size_t i, count;
		char **l;

		count = config_array_count(node);
		l = NULL;
		for (i = 0; i < count; i++)
			l = sl_append_printf(l, "%zu", i);
		return l;
	} else {
		return NULL;
	}
}

bool cli_config_key_validate(struct config_node *parent, const char *key)
{
	return schema_object_key_validate(config_node_schema(parent), key);
}

static bool cli_config_int_validate(struct schema_node *schema, const char *val, long *ret)
{
	char *endptr;
	long i;

	i = strtol(val, &endptr, 0);
	if (i == LONG_MIN || i == LONG_MAX || *endptr != 0)
		return false;
	if (!schema_int_validate(schema, i))
		return false;
	if (ret)
		*ret = i;
	return true;
}

static bool cli_config_number_validate(struct schema_node *schema, const char *val, double *ret)
{
	char *endptr;
	double d;

	d = strtod(val, &endptr);
	if (d == HUGE_VALF || d == HUGE_VALL || isnan(d) || isinf(d) || *endptr != 0)
		return false;
	if (ret)
		*ret = d;
	return true;
}

static bool cli_config_bool_validate(const char *val, bool *ret)
{
	if (strcmp(val, "true") == 0 || strcmp(val, "yes") == 0 || strcmp(val, "1") == 0) {
		if (ret)
			*ret = true;
		return true;
	}

	if (strcmp(val, "false") == 0 || strcmp(val, "no") == 0 || strcmp(val, "0") == 0) {
		if (ret)
			*ret = false;
		return true;
	}

	return false;
}

static bool cli_config_string_validate(struct config_node *node,
	const char *val, bool strict_ref)
{
	struct schema_node *schema = config_node_schema(node);

	if (!strict_ref) {
		char **refs, **ref;

		if (schema_string_ref(schema))
			return schema_object_key_validate(schema, val);

		refs = schema_string_refs(schema);
		if (refs) {
			for (ref = refs; *ref; ref++) {
				const char *key;

				if (fnmatch(*ref, val, FNM_PATHNAME) != 0)
					continue;

				key = strrchr(val, '/');
				if (key != NULL)
					key++;
				else
					key = val;
				return schema_object_key_validate(schema, key);
			}
			return false;
		}
	}

	return schema_string_validate(schema, config_node_parent(node), val);
}

static char **cli_config_bool_values(void)
{
	return sl_new("true", "false", "yes", "no", "1", "0");
}

char **cli_config_node_values(struct config_node *node)
{
	switch (config_node_type(node)) {
	case CONFIG_TYPE_ROOT:
	case CONFIG_TYPE_OBJECT:
	case CONFIG_TYPE_ARRAY:
	case CONFIG_TYPE_ALIAS:
	case CONFIG_TYPE_INT:
	case CONFIG_TYPE_NUMBER:
		break;
	case CONFIG_TYPE_STRING:
		return schema_string_values(config_node_schema(node), config_node_parent(node));
	case CONFIG_TYPE_TRUE:
	case CONFIG_TYPE_FALSE:
		return cli_config_bool_values();
	}

	return NULL;
}

bool cli_config_node_validate(struct config_node *node, const char *val,
	bool strict_ref)
{
	switch (config_node_type(node)) {
	case CONFIG_TYPE_ROOT:
	case CONFIG_TYPE_OBJECT:
	case CONFIG_TYPE_ARRAY:
	case CONFIG_TYPE_ALIAS:
		break;
	case CONFIG_TYPE_STRING:
		return cli_config_string_validate(node, val, strict_ref);
	case CONFIG_TYPE_INT:
		return cli_config_int_validate(config_node_schema(node), val, NULL);
	case CONFIG_TYPE_NUMBER:
		return cli_config_number_validate(config_node_schema(node), val, NULL);
	case CONFIG_TYPE_TRUE:
	case CONFIG_TYPE_FALSE:
		return cli_config_bool_validate(val, NULL);
	}

	return false;
}

static struct schema_node *cli_config_container_schema(struct config_node *node)
{
	if (config_is_object(node)) {
		return config_object_additional_schema(node);
	} else if (config_is_array(node)) {
		return config_array_items_schema(node);
	} else {
		return NULL;
	}
}

bool cli_config_container_contains_fields(struct config_node *node)
{
	struct schema_node *schema;

	schema = cli_config_container_schema(node);
	if (!schema)
		return false;

	switch (schema_node_type(schema)) {
	case SCHEMA_TYPE_STRING:
	case SCHEMA_TYPE_INT:
	case SCHEMA_TYPE_NUMBER:
	case SCHEMA_TYPE_BOOL:
		return true;
		break;
	default:
		return false;
	}
}

char **cli_config_container_values(struct config_node *node)
{
	struct schema_node *schema;

	schema = cli_config_container_schema(node);
	if (!schema)
		return NULL;

	switch (schema_node_type(schema)) {
	case SCHEMA_TYPE_OBJECT:
	case SCHEMA_TYPE_ARRAY:
	case SCHEMA_TYPE_ALIAS:
	case SCHEMA_TYPE_INT:
	case SCHEMA_TYPE_NUMBER:
		break;
	case SCHEMA_TYPE_STRING:
		return schema_string_values(schema, node);
	case SCHEMA_TYPE_BOOL:
		return cli_config_bool_values();
	}

	return NULL;
}

bool cli_config_container_validate(struct config_node *node, const char *val)
{
	struct schema_node *schema;

	schema = cli_config_container_schema(node);
	if (!schema)
		return false;

	switch (schema_node_type(schema)) {
	case SCHEMA_TYPE_OBJECT:
	case SCHEMA_TYPE_ARRAY:
	case SCHEMA_TYPE_ALIAS:
		break;
	case SCHEMA_TYPE_STRING:
		return schema_string_validate(schema, node, val);
	case SCHEMA_TYPE_INT:
		return cli_config_int_validate(schema, val, NULL);
	case SCHEMA_TYPE_NUMBER:
		return cli_config_number_validate(schema, val, NULL);
	case SCHEMA_TYPE_BOOL:
		return cli_config_bool_validate(val, NULL);
	}

	return false;
}


static char *cli_config_node_get_string(struct config_node *node, bool escaped)
{
	const char *str, *strp;
	char *buf, *bufp;

	str = config_string_get(node);

	/* No need to escape obfuscated private strings, safe format */
	if (config_node_is_private(node))
		return config_private_obfuscate(str, strlen(str));

	if (!escaped)
		return strdup(str);

	/* Escape " and \ */
	buf = malloc(strlen(str)*2 + 1);
	strp = str;
	bufp = buf;
	while (strp && *strp) {
		if (*strp == '\\' || *strp == '"')
			*(bufp++) = '\\';
		*(bufp++) = *(strp++);
	}
	*bufp = 0;

	return buf;
}

char *cli_config_node_get(struct config_node *node, bool redacted, bool escaped)
{
	char *buf;

	if (redacted && config_node_is_private(node))
		return strdup("[private]");

	switch (config_node_type(node)) {
	case CONFIG_TYPE_ROOT:
	case CONFIG_TYPE_OBJECT:
	case CONFIG_TYPE_ARRAY:
	case CONFIG_TYPE_ALIAS:
		break;
	case CONFIG_TYPE_STRING:
		return cli_config_node_get_string(node, escaped);
	case CONFIG_TYPE_INT:
		xasprintf(&buf, "%d", config_int_get(node));
		return buf;
	case CONFIG_TYPE_NUMBER:
		xasprintf(&buf, "%lf", config_number_get(node));
		return buf;
	case CONFIG_TYPE_TRUE:
		return strdup("true");
	case CONFIG_TYPE_FALSE:
		return strdup("false");
	}

	return NULL;
}

void cli_config_node_set(struct config_node *node, const char *val)
{
	long i;
	double d;
	bool b;

	switch (config_node_type(node)) {
	case CONFIG_TYPE_ROOT:
	case CONFIG_TYPE_OBJECT:
	case CONFIG_TYPE_ARRAY:
	case CONFIG_TYPE_ALIAS:
		break;
	case CONFIG_TYPE_STRING: {
		char *nval;
		if (config_node_is_private(node))
			nval = config_private_deobfuscate(val, strlen(val));
		else
			nval = strdup(val);
		config_string_set(node, nval);
		} break;
	case CONFIG_TYPE_INT:
		if (cli_config_int_validate(config_node_schema(node), val, &i))
			config_int_set(node, i);
		break;
	case CONFIG_TYPE_NUMBER:
		if (cli_config_number_validate(config_node_schema(node), val, &d))
			config_number_set(node, d);
		break;
	case CONFIG_TYPE_TRUE:
	case CONFIG_TYPE_FALSE:
		if (cli_config_bool_validate(val, &b))
			config_bool_set(node, b);
		break;
	}
}

struct config_node *cli_config_lookup(struct config_node *parent, const char *path)
{
	struct config_node *node = parent;
	char **l;
	char **key;

	l = sl_split(path, '.');
	for (key = l; *key; key++) {
		node = config_lookup_single(node, *key);
		if (!node) {
			break;
		}
	}
	sl_free(l);

	return node;
}

static char **cli_config_cmd_node(struct config_node *node,
	struct config_node *node_default);

static char **cli_config_cmd_value(struct config_node *node)
{
	char **cmds;
	char **sl;
	char *path, *v;

	sl = config_path_sl(node);
	path = sl_join(sl, " ");
	sl_free(sl);

	v = cli_config_node_get(node, false, true);
	cmds = sl_append_printf(NULL, "%s \"%s\"", path, v);
	free(v);

	free(path);
	return cmds;
}

static char **cli_config_cmd_object(struct config_node *node,
	struct config_node *node_default)
{
	char **cmds = sl_new(NULL);
	struct config_node *val, *val_default;
	char *path = NULL;
	char **l, **s;
	char **sl;

	/* Fixed objects including those dependVal'd out */
	l = config_object_keys(node, true, false, true);
	for (s = l; *s; s++) {
		val = config_object_get(node, *s);
		val_default = node_default ? config_object_get(node_default, *s) : NULL;
		if (val_default ? !config_node_is_equal(val, val_default)
				: !config_node_is_default(val))
			cmds = sl_concat(cmds, cli_config_cmd_node(val, val_default));
	}
	sl_free(l);

	/* Add dynamic objects including those dependVal'd out */
	l = config_object_keys(node, false, true, true);
	for (s = l; *s; s++) {
		if (!path) {
			sl = config_path_sl(node);
			path = sl_join(sl, " ");
			sl_free(sl);
		}
		val = config_object_get(node, *s);

		if(config_is_string(val)) {
			cmds = sl_append_printf(cmds, "add %s %s \"%s\"", path, *s, config_string_get(val));
		} else {
			cmds = sl_append_printf(cmds, "add %s %s", path, *s);
		}
		cmds = sl_append(cmds, "...");
		cmds = sl_concat(cmds, cli_config_cmd_node(val, NULL));

	}
	sl_free(l);

	free(path);
	return cmds;
}

static char **cli_config_cmd_array(struct config_node *node,
	struct config_node *node_default)
{
	char **cmds = sl_new(NULL);
	size_t i, count, count_defaults;
	struct config_node *val, *val_default;
	struct schema_node *schema;
	char **sl;
	char *path, *v;

	sl = config_path_sl(node);
	path = sl_join(sl, " ");
	sl_free(sl);

	schema = config_node_schema(node);
	node_default = node_default ?: schema_node_default(schema);

	count = config_array_count_internal(node);
	count_defaults = node_default ? config_array_count_internal(node_default) : 0;

	/* Changed array elements */
	for (i = 0; i < count && i < count_defaults; i++) {
		val = config_array_get(node, i);
		val_default = config_array_get(node_default, i);
		if (!config_node_is_equal(val, val_default)) {
			cmds = sl_concat(cmds, cli_config_cmd_node(val, val_default));
		}
	}

	/* Add new array elements */
	for (; i < count; i++) {
		val = config_array_get(node, i);
		switch (config_node_type(val)) {
		case CONFIG_TYPE_OBJECT:
		case CONFIG_TYPE_ARRAY:
			/* Array items are complex. Must add then edit. */
			cmds = sl_append_printf(cmds, "add %s end", path);
			cmds = sl_append(cmds, "...");
			cmds = sl_concat(cmds, cli_config_cmd_node(val, NULL));
			break;
		default:
			/* Array items are simple string/int/etc. Must set during add. */
			v = cli_config_node_get(val, false, true);
			cmds = sl_append_printf(cmds, "add %s end \"%s\"", path, v);
			free(v);
			break;
		}
	}

	/* Remove trailing default elements */
	for (; i < count_defaults; i++)
		cmds = sl_append_printf(cmds, "del %s %d", path, count);

	free(path);
	return cmds;
}

static char **cli_config_cmd_node(struct config_node *node,
	struct config_node *node_default)
{
	switch (config_node_type(node)) {
	case CONFIG_TYPE_ROOT:
		return NULL;
		break;
	case CONFIG_TYPE_OBJECT:
		return cli_config_cmd_object(node, node_default);
	case CONFIG_TYPE_ARRAY:
		return cli_config_cmd_array(node, node_default);
	default:
		return cli_config_cmd_value(node);
	}
	return sl_new(NULL);
}

char *cli_config_commands(struct config_node *node)
{
	char **sl = NULL;
	char *cmds;
	char *info;
	char *s;

	info = file_read("/etc/version");
	if ((s = strchr(info, '\n')))
		*s = 0;

	sl = sl_append(sl, "# Configuration for firmware:");
	sl = sl_append_printf(sl, "# %s", info);
	sl = sl_append(sl, "#", "config", "revert");

	sl = sl_concat(sl, cli_config_cmd_node(node, NULL));

	sl = sl_append(sl, "save", "");
	cmds = sl_join(sl, "\n");
	sl_free(sl);
	return cmds;
}

static const char *cli_config_help_format(struct schema_node *schema)
{
	const char *format = schema_string_format(schema);

	if (!format)
		return NULL;

	return schema_format_syntax_lookup(format);
}

static void string_trunc_ellipsis(char *s, int maxlen)
{
	int len = strlen(s);

	if (len <= maxlen)
		return;

	s[maxlen] = '\0';
	s[maxlen - 1] = '.';
	s[maxlen - 2] = '.';
	s[maxlen - 3] = '.';
}

void cli_print_table(
	char **col1, int maxlen1,
	char **col2, int maxlen2,
	char **col3, int maxlen3,
	bool heading)
{
	char **c1, **c2, **c3;
	char *empty = NULL;
	int len1 = 0, len2 = 0, len3 = 0;
	int len;

	if (col1 == NULL) col1 = &empty;
	if (col2 == NULL) col2 = &empty;
	if (col3 == NULL) col3 = &empty;

	/* 2 space padding */
	if (maxlen1 > 2) maxlen1 -= 2;
	if (maxlen2 > 2) maxlen2 -= 2;
	if (maxlen3 > 2) maxlen3 -= 2;

	for (c1 = col1, c2 = col2, c3 = col3;
			*c1 || *c2 || *c3;
			*c1 ? c1++ : c1, *c2 ? c2++ : c2, *c3 ? c3++ : c3) {

		if (*c1) {
			len = strlen(*c1);
			len1 = MAX(len1, len);
		}
		if (*c2) {
			len = strlen(*c2);
			len2 = MAX(len2, len);
		}
		if (*c3) {
			len = strlen(*c3);
			len3 = MAX(len3, len);
		}
	}

	if (maxlen1 > 0) len1 = MIN(len1, maxlen1);
	if (maxlen2 > 0) len2 = MIN(len2, maxlen2);
	if (maxlen3 > 0) len3 = MIN(len3, maxlen3);

	for (c1 = col1, c2 = col2, c3 = col3;
			*c1 || *c2 || *c3;
			*c1 ? c1++ : 0, *c2 ? c2++ : 0, *c3 ? c3++ : 0) {

		if (*c1 && maxlen1 > 0) string_trunc_ellipsis(*c1, len1);
		if (*c2 && maxlen2 > 0) string_trunc_ellipsis(*c2, len2);
		if (*c3 && maxlen3 > 0) string_trunc_ellipsis(*c3, len3);

		cli_printf(" %-*s  %-*s  %-*s\n",
			len1, *c1 ?: "",
			len2, *c2 ?: "",
			len3, *c3 ?: "");

		if (heading && c1 == col1) {
			cli_print(" ");
			cli_print_separator('-', 79);
		}
	}
}

static void cli_print_config_help_list_children(struct config_node *node)
{
	char **field_keys, **cfg_keys;
	char **field_values;
	char **field_titles, **cfg_titles;
	char **keys, **key;
	char *value;
	const char *title;
	struct config_node *child_node;
	struct schema_node *child_schema;

	field_keys = sl_new("Parameters              ");
	cfg_keys =   sl_new("Additional Configuration");

	field_values = sl_new("Current Value");
	field_titles = sl_new("");
	cfg_titles = sl_new("");

	keys = config_keys(node);
	for (key = keys; key && *key; key++) {
		child_node = config_lookup_single(node, *key);

		if (!config_node_depend_check(child_node)
				|| config_node_is_debug(child_node))
			continue;

		child_schema = config_node_schema(child_node);
		title = child_schema ? schema_node_title(child_schema) : "";

		if (cli_config_node_is_field(child_node)) {
			field_keys = sl_append(field_keys, *key);
			field_titles = sl_append(field_titles, title ?: "");
			value = cli_config_node_get(child_node, true, false);
			field_values = sl_append(field_values, value ?: "");
			free(value);
		} else {
			cfg_keys = sl_append(cfg_keys, *key);
			cfg_titles = sl_append(cfg_titles, title ?: "");
		}
	}

	if (sl_len(field_keys) > 1) {
		cli_print("\n");
		cli_print_table(field_keys, -1, field_values, 35, field_titles, -1, true);
	}
	if (sl_len(cfg_keys) > 1) {
		cli_print("\n");
		cli_print_table(cfg_keys, -1, cfg_titles, -1, NULL, -1, true);
	}

	sl_free(keys);
	sl_free(cfg_titles);
	sl_free(cfg_keys);
	sl_free(field_titles);
	sl_free(field_values);
	sl_free(field_keys);
}

void cli_print_config_help(struct config_node *node, bool arg_format)
{
	struct schema_node *schema;
	enum schema_type type;
	struct config_node *def;
	const char *title;
	const char *description;
	char *str;
	char **value, **values;

	schema = config_node_schema(node);
	if (!schema)
		return;

	type = schema_node_type(schema);

	if (arg_format) {
		title = config_key(node);
		description = schema_node_description(schema) ?: schema_node_title(schema);
	} else {
		title = schema_node_title(schema);
		description = schema_node_description(schema);
	}

	if (title) {
		cli_printf("\n%s%s%s\n", title,
			description ? ": " : "",
			description ?: "");
	}

	switch (type) {
	case SCHEMA_TYPE_OBJECT:
		cli_print_config_help_list_children(node);
		break;
	case SCHEMA_TYPE_ARRAY:
		break;
	case SCHEMA_TYPE_STRING:
		description = cli_config_help_format(schema);
		if (description) {
			cli_printf("Format: %s\n", description);
		} else if ((values = schema_string_values(schema, node))) {
			/* TODO: descriptions for enums */
			cli_print("Format:\n");
			for (value = values; *value; value++)
				cli_printf("  %s\n", *value);
			sl_free(values);
		}

		if (schema_string_minimum(schema) != NULL)
			cli_printf("Minimum: %s\n", schema_string_minimum(schema));
		if (schema_string_maximum(schema) != NULL)
			cli_printf("Maximum: %s\n", schema_string_maximum(schema));

		break;
	case SCHEMA_TYPE_INT:
		cli_print("Format: integer\n");
		if (schema_int_minimum(schema) != INT_MIN)
			cli_printf("Minimum: %d\n", schema_int_minimum(schema));
		if (schema_int_maximum(schema) != INT_MAX)
			cli_printf("Maximum: %d\n", schema_int_maximum(schema));

		break;
	case SCHEMA_TYPE_NUMBER:
		cli_print("Format: number\n");
		break;
	case SCHEMA_TYPE_BOOL:
		cli_print("Format: true, false, yes, no, 1, 0\n");
		break;
	case SCHEMA_TYPE_ALIAS:
		break;
	}

	if (schema_node_is_optional(schema))
		cli_print("Optional: yes\n");

	def = schema_node_default(schema);
	if (def) {
		str = cli_config_node_get(def, false, false);
		if (str) {
			cli_printf("Default value: %s\n", str);
			free(str);
		}
	}

	if (!arg_format) {
		str = cli_config_node_get(node, true, false);
		if (str) {
			cli_printf("Current value: %s\n", str);
			free(str);
		}
	}
	cli_print("\n");

	/* TODO: Show array item and additionalProperties help ???? */
}

static int help_word_len(const char *help)
{
	int len = 0;

	while (*help) {
		if (strncmp(help, "<p>", 3) == 0
				|| strncmp(help, "<b>", 3) == 0
				|| strncmp(help, "<i>", 3) == 0) {
			help += 3;
			continue;
		}
		if (strncmp(help, "</b>", 4) == 0
				|| strncmp(help, "</i>", 4) == 0) {
			help += 4;
			continue;
		}
		if (strncmp(help, "&trade;", 7) == 0) {
			help += 7;
			continue;
		}
		if (strncmp(help, "&reg;", 5) == 0) {
			help += 5;
			continue;
		}
		if (*help == '\n' || *help == ' ' || *help == '|'
				|| strncmp(help, "</p><p>", 7) == 0 || strncmp(help, "</p>", 4) == 0)
			break;
		len++;
		help++;
	}

	return len;
}


/* Print to wrap on ' ', '|' */
void cli_print(const char *msg)
{
	int len;
	int current = 0;
	int width = tinyrl__get_width(cli_root->tinyrl);

	while (*msg) {
		if (strncmp(msg, "</p><p>", 7) == 0) {
			fputc('\n', stdout);
			current = 0;
			msg += 7;
			continue;
		}
		if (strncmp(msg, "<p>", 3) == 0
				|| strncmp(msg, "<b>", 3) == 0
				|| strncmp(msg, "<i>", 3) == 0) {
			msg += 3;
			continue;
		}
		if (strncmp(msg, "</p>", 4) == 0
				|| strncmp(msg, "</b>", 4) == 0
				|| strncmp(msg, "</i>", 4) == 0) {
			msg += 4;
			continue;
		}
		if (strncmp(msg, "&trade;", 7) == 0) {
			msg += 7;
			continue;
		}
		if (strncmp(msg, "&reg;", 5) == 0) {
			msg += 5;
			continue;
		}
		if (*msg == '\n') {
			fputc('\n', stdout);
			current = 0;
			msg++;
			continue;
		}
		if (*msg == ' ' || *msg == '|') {
			len = help_word_len(msg + 1);
			if (current + 1 + len > width) {
				fputc('\n', stdout);
				current = 0;
				msg++;
				continue;
			}
		}
		fputc(*msg, stdout);
		current++;
		msg++;
	}
}

void cli_printf(const char *fmt, ...)
{
	int ret;
	char *str;
	va_list ap;

	va_start(ap, fmt);
	ret = vasprintf(&str, fmt, ap);
	va_end(ap);
	if (ret < 0)
		return;

	cli_print(str);
	free(str);
}

void cli_print_separator(char c, int n)
{
	int width = tinyrl__get_width(cli_root->tinyrl);
	if (width > n)
		width = n;
	while (--width >= 0) putc(c, stdout);
	printf("\n");
}
#endif

