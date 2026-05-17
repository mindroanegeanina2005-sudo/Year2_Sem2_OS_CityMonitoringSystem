# AI USED: Deepseek

## Promps given:

This is the report typedef struct Report{

int reportID;

char inspector_name[50];

double GPS_lat;

double GPS_long;

char issue[100];

int severity;

time_t timestamp;

char desc[200];

}Report;

A condition is a single string of the form: field:operator:value. Supported fields: severity, category, inspector, timestamp. Supported operators: ==, !=, <, <=, >, >=. Make   int parse_condition(const char *input, char *field, char *op, char *value);
      which splits a field:operator:value string into its three parts.
 and  int match_condition(Report *r, const char *field, const char *op, const char *value);
      which returns 1 if the record satisfies the condition and 0 otherwise.

## What Was Generated

/**
 * Parses a condition string of the form "field:operator:value".
 *
 * @param input The input string (e.g., "severity:>=:3").
 * @param field Output buffer for the field name.
 * @param op    Output buffer for the operator.
 * @param value Output buffer for the value.
 * @return 0 on success, -1 on invalid format.
 */
int parse_condition(const char *input, char *field, char *op, char *value) {
    if (!input || !field || !op || !value)
        return -1;

    // Find first colon
    const char *first_colon = strchr(input, ':');
    if (!first_colon)
        return -1;

    // Extract field (before first colon)
    size_t field_len = first_colon - input;
    if (field_len == 0)
        return -1;
    strncpy(field, input, field_len);
    field[field_len] = '\0';

    // Find second colon after the first one
    const char *second_colon = strchr(first_colon + 1, ':');
    if (!second_colon)
        return -1;

    // Extract operator (between first and second colon)
    size_t op_len = second_colon - (first_colon + 1);
    if (op_len == 0 || op_len > 2)   // operator is 1 or 2 characters
        return -1;
    strncpy(op, first_colon + 1, op_len);
    op[op_len] = '\0';

    // Extract value (after second colon)
    const char *val_start = second_colon + 1;
    size_t val_len = strlen(val_start);
    if (val_len == 0)
        return -1;
    strcpy(value, val_start);   // value buffer must be large enough

    return 0;
}

/**
 * Helper: compares two numbers using the given operator string.
 * Returns 1 if the comparison holds, 0 otherwise.
 */
static int compare_numbers(long long a, long long b, const char *op) {
    if (strcmp(op, "==") == 0) return a == b;
    if (strcmp(op, "!=") == 0) return a != b;
    if (strcmp(op, "<") == 0)  return a < b;
    if (strcmp(op, "<=") == 0) return a <= b;
    if (strcmp(op, ">") == 0)  return a > b;
    if (strcmp(op, ">=") == 0) return a >= b;
    return 0;   // unknown operator
}

/**
 * Checks whether a report satisfies the given condition.
 *
 * @param r     Pointer to the report.
 * @param field The field name (severity, inspector, timestamp).
 * @param op    The operator (==, !=, <, <=, >, >=).
 * @param value The value to compare against (string).
 * @return 1 if condition holds, 0 otherwise.
 */
int match_condition(Report *r, const char *field, const char *op, const char *value) {
    if (!r || !field || !op || !value)
        return 0;

    // Severity field (integer)
    if (strcmp(field, "severity") == 0) {
        long long report_val = r->severity;
        long long cond_val = atoll(value);   // convert value to integer
        return compare_numbers(report_val, cond_val, op);
    }

    // Timestamp field (time_t, treated as integer)
    if (strcmp(field, "timestamp") == 0) {
        long long report_val = (long long)r->timestamp;
        long long cond_val = atoll(value);
        return compare_numbers(report_val, cond_val, op);
    }

    // Inspector field (string)
    if (strcmp(field, "inspector") == 0) {
        if (strcmp(op, "==") == 0)
            return strcmp(r->inspector_name, value) == 0;
        if (strcmp(op, "!=") == 0)
            return strcmp(r->inspector_name, value) != 0;
        // For other operators (e.g., <, >) on a string, we return 0
        return 0;
    }

    // "category" is listed in the problem but not present in the struct.
    // According to the spec we could ignore it or treat as false.
    // We return 0 for any unsupported field.
    return 0;
}

## What I Changed and Why

-> Added the category match condition. It was missing.
-> UNIX has problems with > and < > so we change parse_condition. we delete take text between " " and then with that text we continue with it with the original logic given by the AI.
-> Changed a lot of types to easier to understand, long long into int. size_t into char. Aslo removed a lot of const to make it more readable for me.
-> Never used atoll before, so i changed it into a atoi.


## What I Learned

-> AI sometimes misses things , taking things to literaly. Not providing things that do not match what was asked 1 to 1.
-> After testing I figured the terminal sees < > and tried to do something else. I had to go and edit the parse funtion. Something that I had to do by 'cheating' a bit.