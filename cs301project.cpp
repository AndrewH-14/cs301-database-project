#include <algorithm>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>

using namespace std;

/**
 * An enumeration representing the types of data that can be stored in the 
 * database.
*/
enum data_type
{
    CHAR,
    STRING,
    INT,
    FLOAT
};

/**
 * A structure that can store data based on it's type. If no data is available
 * set the empty field to true.
*/
typedef struct data
{
    bool empty;
    char char_data;
    string string_data;
    int int_data;
    float float_data;
} Data;

/**
 * A structure that represents a column in a databases table.
 * 
 * @var column_name The name of the column. Should always be in all caps.
 * @var type        The type of data that the column is storing.
 * @var column_data A vector of Data objects representing the columns data.
*/
typedef struct column
{
    string column_name;
    enum data_type type;
    vector<Data> column_data;
} 
Column;

/**
 * A structure that represents a table in the database.
 * 
 * @var table_name The name of the table. Should always be in all caps.
 * @var A vector of Column objects representing the table's columns.
*/
typedef struct table
{
    string table_name;
    vector<Column> table_data;
} 
Table;

// Implementation functions
vector<Table> init_database(void);
Table parse_table(string where_string, Table table_to_parse);
Table sort_table(string orderby_string, Table table_to_order);
Table select_columns(string select_string, Table table_to_select);
void print_table(Table table_to_print);

// Helper Functions
bool compare_data_condition(Data stored_data, Data test_data, string inequality, enum data_type type);
bool compare_char_condition(Data stored_data, Data test_data, string inequality);
bool compare_string_condition(Data stored_data, Data test_data, string inequality);
bool compare_int_condition(Data stored_data, Data test_data, string inequality);
bool compare_float_condition(Data stored_data, Data test_data, string inequality);
string trim(string string_to_trim);
vector<string> split_string_comma(string string_to_parse);
vector<string> split_string_space(string string_to_parse);
enum data_type get_type(string type_string);

/**
 * The main function of the database program. 
 * 
 * @note To exit the program the user must enter "EXIT"
 * 
 * @param tc_level An integer representing the users permissions in regards to 
 *                 accessing database records.
 * 
 * @retval  0 The program ran successfully.
 * @retval -1 An error was encountered.
*/
int main(int argc, char **argv)
{    
    // Format string to pass to filtering function to ensure the user can only
    // view the appropriate data based on security level.
    string tc_level = "TC<=";
    tc_level.push_back(argv[1][0]);

    // Initialize the database a return a copy to be used for queries
    vector<Table> database = init_database();
    // Loop until the user requests to exit the program
    string input_line;
    while(1)
    {
        cout << "MLS>";
        getline(cin, input_line);

        if (input_line == "EXIT") { break; }

        // Get the query section that comes before the ';' (inclusive)
        size_t pos = input_line.find(';');
        if (pos != string::npos)
        {
            input_line = input_line.substr(0, pos + 1);
        }

        // If ';' is missing the query is invalid
        if (input_line[input_line.size() - 1] == ';')
        {
            // replace the ';' character with a space, so the string can be split
            // properly
            input_line[input_line.size() - 1] = ' ';

            string select_string = "";
            string from_string   = "";
            string where_string  = "";
            string order_string  = "";
            Table copy_of_table;

            // Split the string using a space delimiter in order to parse out
            // query information.
            vector<string> list_of_words = split_string_space(input_line);

            // Use the FROM section to determine which table to use, for this
            // program only one table can be used
            for (int word_idx = 0; word_idx < list_of_words.size() - 1; word_idx++)
            {
                if (list_of_words[word_idx] == "FROM")
                {
                    // First non whitespace string is the table table that we will use
                    from_string = list_of_words[word_idx + 1];
                    break;
                }
            }

            // Get a copy of the table using the FROM statement
            bool table_exists = false;
            for (int table_idx = 0; table_idx < database.size(); table_idx++)
            {
                if (from_string == database[table_idx].table_name)
                {
                    copy_of_table = database[table_idx];
                    table_exists = true;
                    break;
                }
            }
            // If a the table exists, then get the other query information
            if (table_exists)
            {
                // Check for a where statement and use it to parse information if necessary
                int where_idx = 0;
                for (int word_idx = 0; word_idx < list_of_words.size(); word_idx++)
                {
                    if (list_of_words[word_idx] == "WHERE")
                    {
                        where_idx = word_idx;
                        break;
                    }
                }
                // If the where statment exists, then we need to get the conditions
                // that will filter the table
                for (int word_idx = where_idx + 1; word_idx < list_of_words.size(); word_idx++)
                {
                    if (list_of_words[word_idx] != "ORDERBY")
                        where_string = where_string + list_of_words[word_idx] + " ";
                    else
                        break;
                }

                if (where_idx != 0)
                {
                    // Add the users tc level to the filtering string
                    where_string = where_string + "," + tc_level;
                }
                else
                {
                    where_string = tc_level;
                }

                // Parse information out of table using the where string
                copy_of_table = parse_table(where_string, copy_of_table);

                // CHeck for an order by statement
                int orderby_idx = 0;
                for (int word_idx = 0; (word_idx < list_of_words.size()) && (copy_of_table.table_data[0].column_data.size() != 0); word_idx++)
                {
                    if (list_of_words[word_idx] == "ORDERBY")
                    {
                        orderby_idx = word_idx;
                        break;
                    }
                }
                // If the order by statement exists, then we need to get the 
                // conditions that will filter the table
                if (orderby_idx != 0)
                {
                    for (int word_idx = orderby_idx + 1; word_idx < list_of_words.size(); word_idx++)
                    {
                        order_string = order_string + list_of_words[word_idx] + " ";
                    }

                    copy_of_table = sort_table(order_string, copy_of_table);
                }

                // Check for an order by statement
                int select_idx = -1;
                for (int word_idx = 0; word_idx < list_of_words.size(); word_idx++)
                {
                    if (list_of_words[word_idx] == "SELECT")
                    {
                        select_idx = word_idx;
                        break;
                    }
                }

                // If the select statement exists, then we need to get the select conditions,
                // so we know what columns are needed
                if (select_idx != -1)
                {
                    for (int word_idx = select_idx + 1; word_idx < list_of_words.size(); word_idx++)
                    {
                        if (list_of_words[word_idx] != "FROM")
                            select_string = select_string + list_of_words[word_idx] + " ";
                        else
                            break;
                    }
                    copy_of_table = select_columns(select_string, copy_of_table);
                }

                // Print out the entire table, which should only contain the desired
                // elements
                print_table(copy_of_table);
            }
        }
    }

    return 0;
}

/**
 * A function will initialize the database based on the provided TAB_COLUMNS.csv
 * file.
 * 
 * @return A vector of tables representing the database.
*/
vector<Table> init_database()
{
    vector<Table> database;
    string input_line;

    // First need to get the format of the table.
    // The columns should be in descending order.
    ifstream schema_file("TAB_COLUMNS.csv");
    while (getline(schema_file, input_line))
    {
        // Split each line of the schema file. The line should be in the 
        // following order <table>,<column>,<type>,<column_num>
        vector<string> list_of_words = split_string_comma(input_line);

        // Iterate through the database to see if the table already exists
        bool table_exists = false;
        for (int idx = 0; idx < database.size(); idx++)
        {
            // If the table does exist, then just add the column to it
            if (database[idx].table_name == list_of_words[0])
            {
                Column new_column = 
                {
                    .column_name = list_of_words[1],
                    .type = get_type(list_of_words[2])
                };
                database[idx].table_data.push_back(new_column);
                table_exists = true;
                break;
            }
        }
        if (!table_exists) // The table does not yet exist
        {
            Table new_table = { .table_name = list_of_words[0] };
            Column new_column = 
            {
                .column_name = list_of_words[1],
                .type = get_type(list_of_words[2])
            };
            new_table.table_data.push_back(new_column);
            database.push_back(new_table);
        }
    }
    schema_file.close();

    // The schema read from the input file is in decending order, therefore
    // invert the vector so that the columns are indexable.
    for (int table_idx = 0; table_idx < database.size(); table_idx++)
    {
        reverse(database[table_idx].table_data.begin(), database[table_idx].table_data.end());

        // Read through the employee data and store it in the table
        ifstream data_file(database[table_idx].table_name + ".csv");
        while (data_file.is_open() && getline(data_file, input_line))
        {
            vector<string> list_of_words = split_string_comma(input_line);

            for (int column_idx = 0; column_idx < database[table_idx].table_data.size(); column_idx++)
            {
                Data data_item;
                data_item.empty = false;

                enum data_type type = database[table_idx].table_data[column_idx].type;

                // Assign the data value based on the column's type
                if (list_of_words[column_idx] == " ")
                    data_item.empty = true;
                else if (CHAR == type)
                    data_item.char_data = trim(list_of_words[column_idx])[0];
                else if (STRING == type)
                    data_item.string_data = trim(list_of_words[column_idx]);
                else if (INT == type)
                    data_item.int_data = stoi(trim(list_of_words[column_idx]));
                else if (FLOAT == type)
                    data_item.float_data = stof(trim(list_of_words[column_idx]));

                database[table_idx].table_data[column_idx].column_data.push_back(data_item);
            }
        }
        data_file.close();
    }

    return database;
}

/**
 * Function that will filter the given table based on the passed string of 
 * conditions
 * 
 * @param where_string   A string that contains the comma separated list of 
 *                       conditions.
 * @param table_to_parse A Table object that should be filtered.
 * 
 * @return The filtered table.
*/
Table parse_table(string where_string, Table table_to_parse)
{
    vector<string> conditions;

    // Split the given where string based on the commas, which will give us the
    // number of filters that need to be ran.
    conditions = split_string_comma(where_string);

    // For each condition, filter the table
    for (string condition_string : conditions)
    {
        string data1;
        string data2;
        string condition;
        Column column_to_parse;
        Data test_data;
        int current_item = 0;

        // Get the column, inequalty, and data from the condition string
        for (int idx = 0; idx < condition_string.size(); idx++)
        {
            if ((current_item == 0) && (condition_string[idx] != ' ') && 
                (condition_string[idx] != '=') && (condition_string[idx] != '>') && (condition_string[idx] != '<'))
            {
                data1 += condition_string[idx];
            }

            if ((condition_string[idx] == '=') || (condition_string[idx] == '>') || (condition_string[idx] == '<'))
            {
                current_item = 2;
                condition += condition_string[idx];
            } 
            else if ((current_item == 2) && (condition_string[idx] != ' ') && (condition_string[idx] != '\t'))
            {
                data2 += condition_string[idx];
            }
        }

        // Get the column that the condition will be run against
        for (Column column : table_to_parse.table_data)
        {
            if (column.column_name == data1)
            {
                column_to_parse = column;
                if (column_to_parse.type == CHAR)
                    test_data.char_data = data2[0];
                else if (column_to_parse.type == STRING)
                    test_data.string_data = data2;
                else if (column_to_parse.type == INT)
                    test_data.int_data = stoi(data2);
                else // FLOAT
                    test_data.float_data = stof(data2);
            }
        }

        // Go through the column and delete the rows that don't meet the condition
        // from the entire table
        for (int idx = 0; idx < column_to_parse.column_data.size(); idx++)
        {
            if (!compare_data_condition(column_to_parse.column_data[idx], test_data, condition, column_to_parse.type))
            {
                for (int idx2 = 0; idx2 < table_to_parse.table_data.size(); idx2++)
                {
                    table_to_parse.table_data[idx2].column_data.erase(table_to_parse.table_data[idx2].column_data.begin() + idx);
                }
                column_to_parse.column_data.erase(column_to_parse.column_data.begin() + idx);
                idx = idx - 1;
            }
        }
    }   
    return table_to_parse;
}

/**
 * Function that will sort the given table based on the passed string of 
 * conditions
 * 
 * @param orderby_string A string that contains the comma separated list of 
 *                       orderby conditions.
 * @param table_to_parse A Table object that should be sorted.
 * 
 * @return The sorted table.
*/
Table sort_table(string orderby_string, Table table_to_order)
{
    vector<string> order_list = split_string_comma(orderby_string);

    vector<vector<int>> groups;
    vector<int> initial_group;
    for (int data_idx = 0; data_idx < table_to_order.table_data[0].column_data.size(); data_idx++)
    {   
        initial_group.push_back(data_idx);
    }
    groups.push_back(initial_group);

    for (string order_string : order_list)
    {
        string data1;
        string data2;
        Column column_to_order;
        int current_item = 0;

        // Get the column and whether the sorting is to be ascending or descending
        for (int idx = 0; idx < order_string.size(); idx++)
        {
            if ((current_item == 0) && (order_string[idx] != ' ')&& (order_string[idx] != ':'))
                data1 += order_string[idx];
            else if (order_string[idx] == ':')
                current_item = 2;
            else if ((current_item == 2) && (order_string[idx] != ' '))
                data2 += order_string[idx];
        }

        // Get the column that the condition will be run against
        for (Column column : table_to_order.table_data)
        {
            if (column.column_name == data1)
                column_to_order = column;
        }

        string operator_string;
        if (data2 == "1")
            operator_string = ">";
        else if (data2 == "-1")
            operator_string = "<";

        // Sort the table
        for (int data_idx = 0; data_idx < column_to_order.column_data.size() - 1; data_idx++)
        {
            bool data1_in_group = false;
            bool data2_in_group = false;
            for (int group_idx = 0; group_idx < groups.size(); group_idx++)
            {
                if (count(groups[group_idx].begin(), groups[group_idx].end(), data_idx))
                    data1_in_group = true;
                if (count(groups[group_idx].begin(), groups[group_idx].end(), data_idx + 1))
                    data2_in_group = true;

                if (!data1_in_group || !data2_in_group)
                {
                    data1_in_group = false;
                    data2_in_group = false;
                }
                else
                {
                    break;
                }
            }

            if (data1_in_group && data2_in_group)
            {
                if (compare_data_condition(column_to_order.column_data[data_idx], 
                                           column_to_order.column_data[data_idx + 1],
                                           operator_string,
                                           column_to_order.type) &&
                    !column_to_order.column_data[data_idx + 1].empty)
                {
                    // Swap the column values that we are using to sort the table
                    iter_swap(column_to_order.column_data.begin() + data_idx,
                              column_to_order.column_data.begin() + data_idx + 1);

                    // Swap the column values in each column of the table
                    for (int column_idx = 0; column_idx < table_to_order.table_data.size(); column_idx++)
                    {
                        iter_swap(table_to_order.table_data[column_idx].column_data.begin() + data_idx, 
                                  table_to_order.table_data[column_idx].column_data.begin() + data_idx + 1);
                    }

                    // Reset the index back to zero once the for loop execute
                    data_idx = -1;
                }
                else if (column_to_order.column_data[data_idx].empty)
                {
                    // If the next entry is not empty, then move the row with the
                    // empty element down
                    if (!column_to_order.column_data[data_idx + 1].empty)
                    {
                        // Swap the column values that we are using to sort the table
                        iter_swap(column_to_order.column_data.begin() + data_idx,
                                  column_to_order.column_data.begin() + data_idx + 1);

                        // Swap the column values in each column of the table
                        for (int column_idx = 0; column_idx < table_to_order.table_data.size(); column_idx++)
                        {
                            iter_swap(table_to_order.table_data[column_idx].column_data.begin() + data_idx, 
                                      table_to_order.table_data[column_idx].column_data.begin() + data_idx + 1);
                        }
                        // Reset the index back to zero once the for loop execute
                        data_idx = -1;
                    }

                }
            }
        }

        // Set up groups based on if the column to compare has the same value
        vector<vector<int>> new_groups;
        for (int data_idx = 0; data_idx < column_to_order.column_data.size(); data_idx++)
        {
            bool found_group = false;
            for (int group_idx = 0; group_idx < new_groups.size(); group_idx++)
            {
                if (compare_data_condition(column_to_order.column_data[new_groups[group_idx][0]], 
                                           column_to_order.column_data[data_idx],
                                           "=",
                                           column_to_order.type))
                {
                    // Check if the one of the new groups idxes is in the same old group 
                    // as the current data_idx
                    for (int old_group_idx = 0; old_group_idx < groups.size(); old_group_idx++)
                    {
                        if (count(groups[old_group_idx].begin(), groups[old_group_idx].end(), data_idx &&
                            count(groups[old_group_idx].begin(), groups[old_group_idx].end(), new_groups[group_idx][0])))
                        {
                            new_groups[group_idx].push_back(data_idx);
                            found_group = true;
                        }
                    }
                }
            }

            if (!found_group)
            {
                vector<int> group;
                group.push_back(data_idx);
                new_groups.push_back(group);
            }
        }

        groups = new_groups;
    }
    return table_to_order;
}

/**
 * Function that will parse the select statment, and perform the desired
 * operations.
 * 
 * @param select_string   The string that contains the select statement.
 * @param table_to_select The table that the select statement should be run on.
 * 
 * @return Table with only the desired columns.
*/
Table select_columns(string select_string, Table table_to_select)
{
    vector<string> select_list = split_string_comma(select_string);
    Table new_table;
    bool add_column = false;
    vector<string> columns_to_include_or_remove;

    for (string select_string: select_list)
    {
        string data1;
        string data2;
        int current_item = 0;

        // Get the column and whether the sorting is to be ascending or descending
        for (int idx = 0; idx < select_string.size(); idx++)
        {
            if ((current_item == 0) && (select_string[idx] != ' ') && (select_string[idx] != ':'))
            {
                if (select_string[idx] == '*')
                    return table_to_select;
                data1 += select_string[idx];
            }
            else if (select_string[idx] == ':')
                current_item = 2;
            else if ((current_item == 2) && (select_string[idx] != ' '))
                data2 += select_string[idx];
        }

        if (data2 == "1")
            add_column = true;

        columns_to_include_or_remove.push_back(data1);
    }

    // Include columns based on the list
    if (add_column)
    {
        for (int idx = 0; idx < columns_to_include_or_remove.size(); idx++)
        {
            for (int column_idx = 0; column_idx < table_to_select.table_data.size(); column_idx++)
            {
                if (table_to_select.table_data[column_idx].column_name == columns_to_include_or_remove[idx])
                    new_table.table_data.push_back(table_to_select.table_data[column_idx]);
            }
        }
    }
    else // Remove columns based on the list
    {
        for (int column_idx = 0; column_idx < table_to_select.table_data.size(); column_idx++)
        {
            if (0 == count(columns_to_include_or_remove.begin(),
                           columns_to_include_or_remove.end(),
                           table_to_select.table_data[column_idx].column_name))
            {
                new_table.table_data.push_back(table_to_select.table_data[column_idx]);
            }
        }
    }

    return new_table;
}

/**
 * Prints out the table to stdin.
 * 
 * @param table_to_print The table to print.
*/
void print_table(Table table_to_print)
{
    // Print the column names in a comma seperated list
    for (int column_idx = 0; column_idx < table_to_print.table_data.size(); column_idx++)
    {
        cout << table_to_print.table_data[column_idx].column_name;
        if (column_idx == table_to_print.table_data.size() - 1)
            cout << endl;
        else
            cout << ",";
    }

    // Print each row of data in the table
    for (int row_idx = 0; row_idx < table_to_print.table_data[0].column_data.size(); row_idx++)
    {
        for (int column_idx = 0; column_idx < table_to_print.table_data.size(); column_idx++)
        {
            if (table_to_print.table_data[column_idx].column_data[row_idx].empty)
                cout << "";
            else if (table_to_print.table_data[column_idx].type == CHAR)
                cout << table_to_print.table_data[column_idx].column_data[row_idx].char_data;
            else if (table_to_print.table_data[column_idx].type == STRING)
                cout << table_to_print.table_data[column_idx].column_data[row_idx].string_data;
            else if (table_to_print.table_data[column_idx].type == INT)
                cout << table_to_print.table_data[column_idx].column_data[row_idx].int_data;
            else if (table_to_print.table_data[column_idx].type == FLOAT)
                cout << table_to_print.table_data[column_idx].column_data[row_idx].float_data;

            if (column_idx == table_to_print.table_data.size() - 1)
                cout << endl;
            else
                cout << ",";
        }
    }
    cout << endl;
}

/**
 * Parse a space seperated string of words.
 * 
 * @param string_to_parse THe string of space seperated words.
 * 
 * @return A vector of parsed words.
*/
vector<string> split_string_space(string string_to_parse)
{
    vector<string> result;
    string new_string = "";
    for (int char_idx = 0; char_idx < string_to_parse.size(); char_idx++)
    {
        if (string_to_parse[char_idx] == ' ' || string_to_parse[char_idx] == '\t')
        {
            // Loop and remove any whitespace
            while (string_to_parse[char_idx] == ' ' || string_to_parse[char_idx] == '\t')
            {
                char_idx++;
            }
            char_idx--; // For loop will hanle the last increment
            result.push_back(new_string);
            new_string = "";
        }
        else
        {
            new_string.push_back(string_to_parse[char_idx]);
        }
    }
    return result;
}

/**
 * Parse a comma seperated string of words.
 * 
 * @param string_to_parse The string of comma seperated words.
 * 
 * @return A vector of the parsed words.
*/
vector<string> split_string_comma(string string_to_parse)
{
    vector<string> result;
    // Create string stream from the string
    stringstream s_stream(string_to_parse);
    while(s_stream.good()) 
    {
        string substr;
        // Get string delimited by comma
        getline(s_stream, substr, ',');
        result.push_back(substr);
    }

    return result;
}

/**
 * Converts a type string to its appropiated enum value
 * 
 * @param type_string Enum type in string form.
 * 
 * @return The enum value of the string.
*/
enum data_type get_type(string type_string)
{
    if ("CHAR" == type_string)
        return CHAR;
    else if ("STRING" == type_string)
        return STRING;
    else if("INT" == type_string)
        return INT;
    else if ("FLOAT" == type_string)
        return FLOAT;
    else
    {
        cout << "Invalid type parsed!!!" << endl;
        return STRING;
    }
}

/**
 * Compares the two data items based on the given inequality.
 * 
 * @param stored_data The value that will be on the left hand side of the inequality.
 * @param test_data   The value that will be on the right hand side of the inequality.
 * @param inequality  The operators that will be used in the inequality.
 * 
 * @return The result of the inequality.
*/
bool compare_data_condition(Data stored_data, Data test_data, string inequality, enum data_type type)
{
    if (stored_data.empty)
        return false;
    else if (type == CHAR)
        return compare_char_condition(stored_data, test_data, inequality);
    else if (type == STRING)
        return compare_string_condition(stored_data, test_data, inequality);
    else if (type == INT)
        return compare_int_condition(stored_data, test_data, inequality);
    else if (type == FLOAT)
        return compare_float_condition(stored_data, test_data, inequality);
    else
        return false;
}
bool compare_char_condition(Data stored_data, Data test_data, string inequality)
{
    if (inequality == ">")
        return stored_data.char_data > test_data.char_data;
    else if (inequality == "<")
        return stored_data.char_data < test_data.char_data;
    else if (inequality == "=")
        return stored_data.char_data == test_data.char_data;
    else if (inequality == "<>")
        return stored_data.char_data != test_data.char_data;
    else if (inequality == ">=")
        return stored_data.char_data >= test_data.char_data;
    else if (inequality == "<=")
        return stored_data.char_data <= test_data.char_data;
    else
        return false;
}
bool compare_string_condition(Data stored_data,Data test_data, string inequality)
{
    if (inequality == ">")
        return stored_data.string_data > test_data.string_data;
    else if (inequality == "<")
        return stored_data.string_data < test_data.string_data;
    else if (inequality == "=")
        return stored_data.string_data == test_data.string_data;
    else if (inequality == "<>")
        return stored_data.string_data != test_data.string_data;
    else if (inequality == ">=")
        return stored_data.string_data >= test_data.string_data;
    else if (inequality == "<=")
        return stored_data.string_data <= test_data.string_data;
    else
        return false;
}
bool compare_int_condition(Data stored_data, Data test_data, string inequality)
{
    if (inequality == ">")
        return stored_data.int_data > test_data.int_data;
    else if (inequality == "<")
        return stored_data.int_data < test_data.int_data;
    else if (inequality == "=")
        return stored_data.int_data == test_data.int_data;
    else if (inequality == "<>")
        return stored_data.int_data != test_data.int_data;
    else if (inequality == ">=")
        return stored_data.int_data >= test_data.int_data;
    else if (inequality == "<=")
        return stored_data.int_data <= test_data.int_data;
    else
        return false;
}
bool compare_float_condition(Data stored_data, Data test_data, string inequality)
{
    if (inequality == ">")
        return stored_data.float_data > test_data.float_data;
    else if (inequality == "<")
        return stored_data.float_data < test_data.float_data;
    else if (inequality == "=")
        return stored_data.float_data == test_data.float_data;
    else if (inequality == "<>")
        return stored_data.float_data != test_data.float_data;
    else if (inequality == ">=")
        return stored_data.float_data >= test_data.float_data;
    else if (inequality == "<=")
        return stored_data.float_data <= test_data.float_data;
    else
        return false;
}

/**
 * Function to trim any leading or trailing whitespace from a string.
 * 
 * @param string_to_trim The string that we want to be trimmed.
 * 
 * @return The trimmed string.
*/
string trim(string string_to_trim)
{
    size_t p = string_to_trim.find_first_not_of(" \t");
    string_to_trim.erase(0, p);
   
    p = string_to_trim.find_last_not_of(" \t");
    if (string::npos != p)
         string_to_trim.erase(p+1);
    return string_to_trim;
}