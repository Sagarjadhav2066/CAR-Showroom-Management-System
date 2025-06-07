#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <time.h>

#define MAX_KEY_LENGTH 20
#define MAX_FUEL_TYPE_LENGTH 20
#define MAX_MOBILE_LENGTH 15
#define MAX_ADDRESS_LENGTH 20
#define MAX_REG_NUMBER_LENGTH 20
#define MAX_PAYMENT_TYPE_LENGTH 20
#define MAX_MANUFACTURER_LENGTH 50
#define MAX_SHOWROOMS 10
#define MAX_NAME_LENGTH 50
#define MAX_ADDR_LENGTH 100
#define MAX_VIN_LENGTH 20
#define MAX_COLOR_LENGTH 20
#define MAX_MODEL_LENGTH 30
#define MAX_FUEL_LENGTH 15
#define MAX_TYPE_LENGTH 10
#define MAX_REG_LENGTH 20
#define MAX_PAYMENT_LENGTH 10
#define B_TREE_ORDER 5  



typedef struct {
    char vin[MAX_VIN_LENGTH];
    char model[MAX_MODEL_LENGTH];
    char color[MAX_COLOR_LENGTH];
    float price;
    char fuel_type[MAX_FUEL_LENGTH];
    char type[MAX_TYPE_LENGTH];
    bool is_sold;
    int showroom_id;
} Car;

typedef struct {
    int id;
    char name[MAX_NAME_LENGTH];
    int showroom_id;
    float target;             
    float achieved;           
    float commission;         
} SalesPerson;

typedef struct {
    int id;
    char name[MAX_NAME_LENGTH];
    char mobile[MAX_NAME_LENGTH];
    char address[MAX_ADDR_LENGTH];
    char car_vin[MAX_VIN_LENGTH];
    char registration_number[MAX_REG_LENGTH];
    char payment_type[MAX_PAYMENT_LENGTH]; 
    float down_payment;
    int emi_months;
    float interest_rate;
    int sales_person_id;
} Customer;

typedef struct {
    int id;
    char name[MAX_NAME_LENGTH];
    char manufacturer[MAX_NAME_LENGTH];
    int total_cars;
    int available_cars;
    int sold_cars;
} Showroom;

typedef struct BTreeNode {
    bool is_leaf;
    int key_count;
    char keys[B_TREE_ORDER - 1][MAX_VIN_LENGTH];  
    void* data[B_TREE_ORDER - 1];                 
    struct BTreeNode* children[B_TREE_ORDER];
} BTreeNode;

typedef struct {
    BTreeNode* root;
    int type;
} BTree;

BTreeNode* create_node(bool is_leaf);
BTree* init_btree(int type);
void split_child(BTreeNode* parent, int index, BTreeNode* child);
void insert_non_full(BTreeNode* node, char* key, void* data);
void insert_to_btree(BTree* tree, char* key, void* data);
void* search_btree(BTreeNode* node, char* key);
void print_btree(BTreeNode* node, int level);
void save_cars_to_file(BTreeNode* node, FILE* file);
void save_salespeople_to_file(BTreeNode* node, FILE* file);
void save_customers_to_file(BTreeNode* node, FILE* file);
void save_showrooms_to_file(BTreeNode* node, FILE* file);
void load_from_file(BTree* tree, const char* filename, int type);
void merge_showrooms_sort_by_vin(BTree* car_trees[], int num_showrooms, const char* output_file);
void add_sales_person(BTree* tree, SalesPerson* sp);
Car* find_most_popular_car(BTree* car_trees[], int num_showrooms);
SalesPerson* find_most_successful_sales_person(BTree* sp_trees[], int num_showrooms);
void sell_car(BTree* car_tree, BTree* customer_tree, BTree* showroom_tree, char* vin, Customer* customer, int sales_person_id);
float predict_next_month_sales(BTree* sp_trees[], int num_showrooms);
void display_car_info(BTree* car_tree, char* vin);
void search_sales_persons_by_range(BTree* sp_tree, float min_sales, float max_sales);
void print_customers_with_emi_range(BTree* customer_tree);

BTreeNode* create_node(bool is_leaf) {
    BTreeNode* new_node = (BTreeNode*)malloc(sizeof(BTreeNode));
    if (new_node == NULL) {
        printf("Memory allocation failed\n");
        exit(1);
    }

    new_node->key_count = 0;
    new_node->is_leaf = is_leaf;

    for (int i = 0; i < B_TREE_ORDER; i++) {
        new_node->keys[i][0] = '\0';
        new_node->data[i] = NULL;
        new_node->children[i] = NULL;
    }
    new_node->children[B_TREE_ORDER] = NULL;

    return new_node;
}

BTree* init_btree(int type) {
    BTree* tree = (BTree*)malloc(sizeof(BTree));
    if (tree == NULL) {
        printf("Memory allocation failed\n");
        exit(1);
    }

    tree->root = create_node(true);
    tree->type = type;

    return tree;
}

void split_child(BTreeNode* parent, int index, BTreeNode* child) {
    if (parent == NULL || child == NULL || index < 0 || index > parent->key_count) {
        printf("Invalid parameters for split_child\n");
        return;
    }

    BTreeNode* new_child = create_node(child->is_leaf);
    int t = (B_TREE_ORDER - 1) / 2;

    new_child->key_count = t;

    for (int i = 0; i < t; i++) {
        if (i + t + 1 < child->key_count) {
            strcpy(new_child->keys[i], child->keys[i + t + 1]);
            new_child->data[i] = child->data[i + t + 1];
        }
    }

    if (!child->is_leaf) {
        for (int i = 0; i <= t; i++) {
            if (i + t + 1 <= child->key_count) {
                new_child->children[i] = child->children[i + t + 1];
            }
        }
    }

    child->key_count = t;

    for (int i = parent->key_count; i > index; i--) {
        parent->children[i + 1] = parent->children[i];
    }

    parent->children[index + 1] = new_child;

    for (int i = parent->key_count - 1; i >= index; i--) {
        strcpy(parent->keys[i + 1], parent->keys[i]);
        parent->data[i + 1] = parent->data[i];
    }

    strcpy(parent->keys[index], child->keys[t]);
    parent->data[index] = child->data[t];

    parent->key_count++;
}


void insert_non_full(BTreeNode* node, char* key, void* data) {
    if (node == NULL || key == NULL) {
        return;
    }

    int i = node->key_count - 1;

    if (node->is_leaf) {
        while (i >= 0 && strcmp(key, node->keys[i]) < 0) {
            if (i + 1 < B_TREE_ORDER) {
                strncpy(node->keys[i + 1], node->keys[i], MAX_KEY_LENGTH - 1);
                node->keys[i + 1][MAX_KEY_LENGTH - 1] = '\0';
                node->data[i + 1] = node->data[i];
            }
            i--;
        }

        if (i + 1 < B_TREE_ORDER) {
            strncpy(node->keys[i + 1], key, MAX_KEY_LENGTH - 1);
            node->keys[i + 1][MAX_KEY_LENGTH - 1] = '\0';
            node->data[i + 1] = data;
            node->key_count++;
        } else {
            printf("Error: Attempt to insert beyond B-tree order in leaf node\n");
        }
    } else {
        while (i >= 0 && strcmp(key, node->keys[i]) < 0) {
            i--;
        }
        i++;

        if (i <= node->key_count && node->children[i] != NULL && 
            node->children[i]->key_count == B_TREE_ORDER - 1) {
            split_child(node, i, node->children[i]);

            if (strcmp(key, node->keys[i]) > 0) {
                i++;
            }
        }

        if (i <= node->key_count && node->children[i] != NULL) {
            insert_non_full(node->children[i], key, data);
        } else {
            printf("Error: Attempt to insert into a NULL child node\n");
        }
    }
}


void insert_to_btree(BTree* tree, char* key, void* data) {
    BTreeNode* root = tree->root;

    if (root->key_count == B_TREE_ORDER - 1) {
        BTreeNode* new_root = create_node(false);
        tree->root = new_root;
        new_root->children[0] = root;
        split_child(new_root, 0, root);
        insert_non_full(new_root, key, data);
    } else {
        insert_non_full(root, key, data);
    }
}

void* search_btree(BTreeNode* node, char* key) {
    if (node == NULL || key == NULL) {
        return NULL;
    }

    int i = 0;

    while (i < node->key_count && strcmp(key, node->keys[i]) > 0) {
        i++;
    }

    if (i < node->key_count && strcmp(key, node->keys[i]) == 0) {
        return node->data[i];
    }

    if (node->is_leaf) {
        return NULL;
    }

    return search_btree(node->children[i], key);
}

void print_btree(BTreeNode* node, int level) {
    if (node != NULL) {
        printf("Level %d: ", level);
        for (int i = 0; i < node->key_count; i++) {
            printf("%s ", node->keys[i]);
        }
        printf("\n");

        if (!node->is_leaf) {
            for (int i = 0; i <= node->key_count; i++) {
                print_btree(node->children[i], level + 1);
            }
        }
    }
}


void save_cars_to_file(BTreeNode* node, FILE* file) {
    if (node == NULL || file == NULL) {
        return;
    }
    for (int i = 0; i < node->key_count; i++) {
        Car* car = (Car*)node->data[i];
        if (car != NULL) {
            fprintf(file, "%s,%s,%s,%.2f,%s,%s,%d,%d\n",
                car->vin, car->model, car->color, car->price,
                car->fuel_type, car->type, car->is_sold, car->showroom_id);
        }
    }
    if (!node->is_leaf) {
        for (int i = 0; i <= node->key_count; i++) {
            save_cars_to_file(node->children[i], file);
        }
    }
}

void save_salespeople_to_file(BTreeNode* node, FILE* file) {
    if (node == NULL || file == NULL) {
        return;
    }

    for (int i = 0; i < node->key_count; i++) {
        SalesPerson* sp = (SalesPerson*)node->data[i];
        if (sp != NULL) {
            fprintf(file, "%d,%s,%d,%.2f,%.2f,%.2f\n",
                sp->id, sp->name, sp->showroom_id,
                sp->target, sp->achieved, sp->commission);
        }
    }

    if (!node->is_leaf) {
        for (int i = 0; i <= node->key_count; i++) {
            save_salespeople_to_file(node->children[i], file);
        }
    }
}

void save_customers_to_file(BTreeNode* node, FILE* file) {
    if (node == NULL || file == NULL) {
        return;
    }

    for (int i = 0; i < node->key_count; i++) {
        Customer* customer = (Customer*)node->data[i];
        if (customer != NULL) {
            fprintf(file, "%d,%s,%s,%s,%s,%s,%s,%.2f,%d,%.2f,%d\n",
                customer->id, customer->name, customer->mobile,
                customer->address, customer->car_vin, customer->registration_number,
                customer->payment_type, customer->down_payment,
                customer->emi_months, customer->interest_rate, customer->sales_person_id);
        }
    }

    if (!node->is_leaf) {
        for (int i = 0; i <= node->key_count; i++) {
            save_customers_to_file(node->children[i], file);
        }
    }
}

void save_showrooms_to_file(BTreeNode* node, FILE* file) {
    if (node != NULL) {
        for (int i = 0; i < node->key_count; i++) {
            Showroom* showroom = (Showroom*)node->data[i];
            fprintf(file, "%d,%s,%s,%d,%d,%d\n",
                showroom->id, showroom->name, showroom->manufacturer,
                showroom->total_cars, showroom->available_cars, showroom->sold_cars);
        }

        if (!node->is_leaf) {
            for (int i = 0; i <= node->key_count; i++) {
                save_showrooms_to_file(node->children[i], file);
            }
        }
    }
}


void load_from_file(BTree* tree, const char* filename, int type) {
    if (tree == NULL || filename == NULL) {
        printf("Invalid parameters for load_from_file\n");
        return;
    }
    
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        printf("Cannot open file %s for reading\n", filename);
        return;
    }
    printf("Successfully opened file %s for reading\n", filename);

    char line[512];
    int section = 0;
    
    while (fgets(line, sizeof(line), file)) {
        if (line[0] == '#' || line[0] == '\n') {
            if (strstr(line, "# Cars")) {
                section = 0;
            } else if (strstr(line, "# Salespeople")) {
                section = 1;
            } else if (strstr(line, "# Customers")) {
                section = 2;
            } else if (strstr(line, "# Showrooms")) {
                section = 3;
            }
            continue;
        }

        size_t len = strlen(line);
        if (len > 0 && line[len-1] == '\n') {
            line[len-1] = '\0';
        }
        
        if (strlen(line) == 0) {
            continue;
        }

        void* data = NULL;
        char key[20] = {0};

        switch (type) {
            case 1: { 
                if (section != 0) continue; 
                Car* car = (Car*)malloc(sizeof(Car));
                if (car == NULL) {
                    printf("Memory allocation failed\n");
                    continue;
                }
                
                memset(car, 0, sizeof(Car));
                data = car;
            
                char* token = strtok(line, ",");
                if (token == NULL) {
                    free(car);
                    continue;
                }
                strncpy(car->vin, token, MAX_VIN_LENGTH - 1);
                car->vin[MAX_VIN_LENGTH - 1] = '\0';
                strncpy(key, token, 19);
                key[19] = '\0';
            
                token = strtok(NULL, ",");
                if (token == NULL) {
                    free(car);
                    continue;
                }
                strncpy(car->model, token, MAX_MODEL_LENGTH - 1);
                car->model[MAX_MODEL_LENGTH - 1] = '\0';
            
                token = strtok(NULL, ",");
                if (token == NULL) {
                    free(car);
                    continue;
                }
                strncpy(car->color, token, MAX_COLOR_LENGTH - 1);
                car->color[MAX_COLOR_LENGTH - 1] = '\0';
            
                token = strtok(NULL, ",");
                if (token == NULL) {
                    free(car);
                    continue;
                }
                car->price = atof(token);
            
                token = strtok(NULL, ",");
                if (token == NULL) {
                    free(car);
                    continue;
                }
                strncpy(car->fuel_type, token, MAX_FUEL_LENGTH - 1);
                car->fuel_type[MAX_FUEL_LENGTH - 1] = '\0';
            
                token = strtok(NULL, ",");
                if (token == NULL) {
                    free(car);
                    continue;
                }
                strncpy(car->type, token, MAX_TYPE_LENGTH - 1);
                car->type[MAX_TYPE_LENGTH - 1] = '\0';
            
                token = strtok(NULL, ",");
                if (token == NULL) {
                    free(car);
                    continue;
                }
                car->is_sold = atoi(token);
            
                token = strtok(NULL, ",");
                if (token == NULL) {
                    free(car);
                    continue;
                }
                car->showroom_id = atoi(token);
            
                insert_to_btree(tree, key, car);
                break;
            }
            case 2: { 
                if (section != 1) continue;
                SalesPerson* sp = (SalesPerson*)malloc(sizeof(SalesPerson));
                if (sp == NULL) {
                    printf("Memory allocation failed\n");
                    continue;
                }
                
                memset(sp, 0, sizeof(SalesPerson));
                data = sp;

                char* token = strtok(line, ",");
                if (token != NULL) {
                    sp->id = atoi(token);
                    sprintf(key, "%d", sp->id);

                    token = strtok(NULL, ",");
                    if (token != NULL) {
                        strncpy(sp->name, token, MAX_NAME_LENGTH - 1);
                        sp->name[MAX_NAME_LENGTH - 1] = '\0';
                    }

                    token = strtok(NULL, ",");
                    if (token != NULL) sp->showroom_id = atoi(token);

                    token = strtok(NULL, ",");
                    if (token != NULL) sp->target = atof(token);

                    token = strtok(NULL, ",");
                    if (token != NULL) sp->achieved = atof(token);

                    token = strtok(NULL, ",");
                    if (token != NULL) sp->commission = atof(token);
                } else {
                    free(sp);
                    continue; 
                }
                break;
            }
            case 3: { 
                if (section != 2) continue; 
                Customer* customer = (Customer*)malloc(sizeof(Customer));
                if (customer == NULL) {
                    printf("Memory allocation failed\n");
                    continue;
                }
                
                memset(customer, 0, sizeof(Customer));
                data = customer;

                char* token = strtok(line, ",");
                if (token != NULL) {
                    customer->id = atoi(token);
                    sprintf(key, "%d", customer->id);

                    token = strtok(NULL, ",");
                    if (token != NULL) {
                        strncpy(customer->name, token, MAX_NAME_LENGTH - 1);
                        customer->name[MAX_NAME_LENGTH - 1] = '\0';
                    }

                    token = strtok(NULL, ",");
                    if (token != NULL) {
                        strncpy(customer->mobile, token, MAX_MOBILE_LENGTH - 1);
                        customer->mobile[MAX_MOBILE_LENGTH - 1] = '\0';
                    }

                    token = strtok(NULL, ",");
                    if (token != NULL) {
                        strncpy(customer->address, token, MAX_ADDRESS_LENGTH - 1);
                        customer->address[MAX_ADDRESS_LENGTH - 1] = '\0';
                    }

                    token = strtok(NULL, ",");
                    if (token != NULL) {
                        strncpy(customer->car_vin, token, MAX_VIN_LENGTH - 1);
                        customer->car_vin[MAX_VIN_LENGTH - 1] = '\0';
                    }

                    token = strtok(NULL, ",");
                    if (token != NULL) {
                        strncpy(customer->registration_number, token, MAX_REG_NUMBER_LENGTH - 1);
                        customer->registration_number[MAX_REG_NUMBER_LENGTH - 1] = '\0';
                    }

                    token = strtok(NULL, ",");
                    if (token != NULL) {
                        strncpy(customer->payment_type, token, sizeof(customer->payment_type) - 1);
                        customer->payment_type[sizeof(customer->payment_type) - 1] = '\0';

                    }

                    token = strtok(NULL, ",");
                    if (token != NULL) customer->down_payment = atof(token);

                    token = strtok(NULL, ",");
                    if (token != NULL) customer->emi_months = atoi(token);

                    token = strtok(NULL, ",");
                    if (token != NULL) customer->interest_rate = atof(token);

                    token = strtok(NULL, ",");
                    if (token != NULL) customer->sales_person_id = atoi(token);
                } else {
                    free(customer);
                    continue;  
                }
                break;
            }

            case 4: { 
                if (section != 3) continue; 
                Showroom* showroom = (Showroom*)malloc(sizeof(Showroom));
                if (showroom == NULL) {
                    printf("Memory allocation failed\n");
                    continue;
                }
                
                memset(showroom, 0, sizeof(Showroom));
                data = showroom;

                char* token = strtok(line, ",");
                if (token != NULL) {
                    showroom->id = atoi(token);
                    sprintf(key, "%d", showroom->id);

                    token = strtok(NULL, ",");
                    if (token != NULL) {
                        strncpy(showroom->name, token, MAX_NAME_LENGTH - 1);
                        showroom->name[MAX_NAME_LENGTH - 1] = '\0';
                    }

                    token = strtok(NULL, ",");
                    if (token != NULL) {
                        strncpy(showroom->manufacturer, token, MAX_MANUFACTURER_LENGTH - 1);
                        showroom->manufacturer[MAX_MANUFACTURER_LENGTH - 1] = '\0';
                    }

                    token = strtok(NULL, ",");
                    if (token != NULL) showroom->total_cars = atoi(token);

                    token = strtok(NULL, ",");
                    if (token != NULL) showroom->available_cars = atoi(token);

                    token = strtok(NULL, ",");
                    if (token != NULL) showroom->sold_cars = atoi(token);
                } else {
                    free(showroom);
                    continue; 
                }
                break;
            }
            default:
                printf("Invalid type\n");
                continue;
        }

        if (strlen(key) >
         0 && data != NULL) {
            insert_to_btree(tree, key, data);
        } else if (data != NULL) {
            free(data); 
        }
    }

    fclose(file);
    printf("Finished reading file %s\n", filename);
}

void add_car(BTree* car_tree, BTree* showroom_tree, const char* filename) {
    Car* new_car = (Car*)malloc(sizeof(Car));
    if (new_car == NULL) {
        printf("Memory allocation failed\n");
        return;
    }

    printf("Enter VIN: ");
    scanf(" %[^\n]", new_car->vin);

    printf("Enter model: ");
    scanf(" %[^\n]", new_car->model);

    printf("Enter color: ");
    scanf(" %[^\n]", new_car->color);

    printf("Enter price (in lakhs): ");
    scanf("%f", &new_car->price);

    printf("Enter fuel type (Petrol/Diesel/CNG/Electric): ");
    scanf(" %[^\n]", new_car->fuel_type);

    printf("Enter type (Hatchback/Sedan/SUV): ");
    scanf(" %[^\n]", new_car->type);

    new_car->is_sold = false;

    printf("Enter showroom ID (1-3): ");
    scanf("%d", &new_car->showroom_id);

    if (new_car->showroom_id < 1 || new_car->showroom_id > 3) {
        printf("Invalid showroom ID\n");
        free(new_car);
        return;
    }

    insert_to_btree(car_tree, new_car->vin, new_car);

    char showroom_key[20];
    sprintf(showroom_key, "%d", new_car->showroom_id);
    Showroom* showroom = (Showroom*)search_btree(showroom_tree->root, showroom_key);
    if (showroom != NULL) {
        showroom->total_cars++;
        showroom->available_cars++;
    }

    FILE* file = fopen(filename, "a");
    if (file != NULL) {
        fprintf(file, "%s,%s,%s,%.2f,%s,%s,%d,%d\n",
               new_car->vin, new_car->model, new_car->color, new_car->price,
               new_car->fuel_type, new_car->type, new_car->is_sold, new_car->showroom_id);
        fclose(file);
    }

    printf("Car added successfully!\n");
}

void merge_showrooms_sort_by_vin(BTree* car_trees[], int num_showrooms, const char* output_file) {
    FILE* file = fopen(output_file, "w");
    if (file == NULL) {
        printf("Cannot open file %s for writing\n", output_file);
        return;
    }

    fprintf(file, "VIN,Model,Color,Price,FuelType,Type,IsSold,ShowroomID\n");

    for (int i = 0; i < num_showrooms; i++) {
        save_cars_to_file(car_trees[i]->root, file);
    }

    fclose(file);
    printf("Merged showrooms data saved to %s\n", output_file);
}

void add_sales_person(BTree* tree, SalesPerson* sp) {
    char key[20];
    sprintf(key, "%d", sp->id);
    insert_to_btree(tree, key, sp);
    printf("Sales person %s added with ID %d\n", sp->name, sp->id);
}
typedef struct {
    char model[MAX_MODEL_LENGTH];
    int count;
} CarCount;

Car* find_most_popular_car(BTree* car_trees[], int num_showrooms) {
    CarCount counts[100] = {0};
    int count_index = 0;

    void count_models(BTreeNode* node, CarCount* counts, int* count_index) {
        if (node != NULL) {
            for (int i = 0; i < node->key_count; i++) {
                Car* car = (Car*)node->data[i];
                if (car != NULL && car->is_sold) {
                    bool found = false;
                    for (int j = 0; j < *count_index; j++) {
                        if (strcmp(counts[j].model, car->model) == 0) {
                            counts[j].count++;
                            found = true;
                            break;
                        }
                    }
                    if (!found && *count_index < 100) {
                        strncpy(counts[*count_index].model, car->model, MAX_MODEL_LENGTH - 1);
                        counts[*count_index].model[MAX_MODEL_LENGTH - 1] = '\0';
                        counts[*count_index].count = 1;
                        (*count_index)++;
                    }
                }
            }
    
            if (!node->is_leaf) {
                for (int i = 0; i <= node->key_count; i++) {
                    count_models(node->children[i], counts, count_index);
                }
            }
        }
    }
    for (int i = 0; i < num_showrooms; i++) {
        count_models(car_trees[i]->root, counts, &count_index);
    }

    int max_count = 0;
    char most_popular[MAX_MODEL_LENGTH] = "";
    for (int i = 0; i < count_index; i++) {
        if (counts[i].count > max_count) {
            max_count = counts[i].count;
            strcpy(most_popular, counts[i].model);
        }
    }

    Car* popular_car = NULL;
    for (int i = 0; i < num_showrooms && popular_car == NULL; i++) {
        BTreeNode* node = car_trees[i]->root;

        Car* find_car_by_model(BTreeNode* node, const char* model) {
            if (node != NULL) {
                for (int i = 0; i < node->key_count; i++) {
                    Car* car = (Car*)node->data[i];
                    if (strcmp(car->model, model) == 0) {
                        return car;
                    }
                }

                if (!node->is_leaf) {
                    for (int i = 0; i <= node->key_count; i++) {
                        Car* found = find_car_by_model(node->children[i], model);
                        if (found != NULL) {
                            return found;
                        }
                    }
                }
            }
            return NULL;
        }

        popular_car = find_car_by_model(node, most_popular);
    }

    printf("Most popular car model: %s (sold %d times)\n", most_popular, max_count);
    return popular_car;
}

SalesPerson* find_most_successful_sales_person(BTree* sp_trees[], int num_showrooms) {
    SalesPerson* best_sp = NULL;
    float max_achieved = 0;

    void find_best_sp(BTreeNode* node) {
        if (node != NULL) {
            for (int i = 0; i < node->key_count; i++) {
                SalesPerson* sp = (SalesPerson*)node->data[i];
                if (sp->achieved > max_achieved) {
                    max_achieved = sp->achieved;
                    best_sp = sp;
                }
            }

            if (!node->is_leaf) {
                for (int i = 0; i <= node->key_count; i++) {
                    find_best_sp(node->children[i]);
                }
            }
        }
    }

    for (int i = 0; i < num_showrooms; i++) {
        find_best_sp(sp_trees[i]->root);
    }

    if (best_sp != NULL) {
        float bonus = best_sp->achieved * 0.01;
        best_sp->commission += bonus;

        printf("Best sales person: %s (ID: %d)\n", best_sp->name, best_sp->id);
        printf("Sales achieved: %.2f lakhs\n", best_sp->achieved);
        printf("Regular commission (2%%): %.2f lakhs\n", best_sp->achieved * 0.02);
        printf("Additional bonus (1%%): %.2f lakhs\n", bonus);
        printf("Total commission: %.2f lakhs\n", best_sp->commission);
    }

    return best_sp;
}

void sell_car(BTree* car_tree, BTree* customer_tree, BTree* showroom_tree, char* vin, Customer* customer, int sales_person_id) {
    Car* car = (Car*)search_btree(car_tree->root, vin);
    if (car == NULL) {
        printf("Car with VIN %s not found\n", vin);
        return;
    }

    if (car->is_sold) {
        printf("Car with VIN %s is already sold\n", vin);
        return;
    }

    car->is_sold = true;

    strcpy(customer->car_vin, vin);
    customer->sales_person_id = sales_person_id;

    char key[20];
    sprintf(key, "%d", customer->id);
    insert_to_btree(customer_tree, key, customer);

    char showroom_key[20];
    sprintf(showroom_key, "%d", car->showroom_id);
    Showroom* showroom = (Showroom*)search_btree(showroom_tree->root, showroom_key);
    if (showroom != NULL) {
        showroom->available_cars--;
        showroom->sold_cars++;
    }

    printf("Car with VIN %s sold to customer %s\n", vin, customer->name);
}

float predict_next_month_sales(BTree* sp_trees[], int num_showrooms) {
    float total_sales = 0;
    int count = 0;

    void calculate_sales(BTreeNode* node) {
        if (node != NULL) {
            for (int i = 0; i < node->key_count; i++) {
                SalesPerson* sp = (SalesPerson*)node->data[i];
                total_sales += sp->achieved;
                count++;
            }

            if (!node->is_leaf) {
                for (int i = 0; i <= node->key_count; i++) {
                    calculate_sales(node->children[i]);
                }
            }
        }
    }

    for (int i = 0; i < num_showrooms; i++) {
        calculate_sales(sp_trees[i]->root);
    }

    float avg_sales = count > 0 ? total_sales / count : 0;
    float predicted_sales = avg_sales * 1.1;

    printf("Average sales per person: %.2f lakhs\n", avg_sales);
    printf("Predicted next month sales: %.2f lakhs\n", predicted_sales);

    return predicted_sales;
}

void display_car_info(BTree* car_tree, char* vin) {
    Car* car = (Car*)search_btree(car_tree->root, vin);
    if (car == NULL) {
        printf("Car with VIN %s not found\n", vin);
        return;
    }

    printf("\nCar Information:\n");
    printf("VIN: %s\n", car->vin);
    printf("Model: %s\n", car->model);
    printf("Color: %s\n", car->color);
    printf("Price: %.2f lakhs\n", car->price);
    printf("Fuel Type: %s\n", car->fuel_type);
    printf("Type: %s\n", car->type);
    printf("Status: %s\n", car->is_sold ? "Sold" : "Available");
    printf("Showroom ID: %d\n", car->showroom_id);
}

void search_sales_persons_by_range(BTree* sp_tree, float min_sales, float max_sales) {
    printf("\nSales persons with sales between %.2f and %.2f lakhs:\n", min_sales, max_sales);

    void search_in_range(BTreeNode* node) {
        if (node != NULL) {
            for (int i = 0; i < node->key_count; i++) {
                SalesPerson* sp = (SalesPerson*)node->data[i];
                if (sp->achieved >= min_sales && sp->achieved <= max_sales) {
                    printf("ID: %d, Name: %s, Sales: %.2f lakhs\n", sp->id, sp->name, sp->achieved);
                }
            }

            if (!node->is_leaf) {
                for (int i = 0; i <= node->key_count; i++) {
                    search_in_range(node->children[i]);
                }
            }
        }
    }

    search_in_range(sp_tree->root);
}

void print_customers_with_emi_range(BTree* customer_tree) {
    printf("\nCustomers with EMI plan between 36 and 48 months:\n");
    void search_emi_range(BTreeNode* node) {
        if (node != NULL) {
            for (int i = 0; i < node->key_count; i++) {
                Customer* customer = (Customer*)node->data[i];
                if (strcmp(customer->payment_type, "Loan") == 0 &&
                    customer->emi_months > 36 && customer->emi_months < 48) {
                    printf("ID: %d, Name: %s, EMI Months: %d, Interest Rate: %.2f%%\n",
                           customer->id, customer->name, customer->emi_months, customer->interest_rate);
                }
            }

            if (!node->is_leaf) {
                for (int i = 0; i <= node->key_count; i++) {
                    search_emi_range(node->children[i]);
                }
            }
        }
    }

    search_emi_range(customer_tree->root);
}

int main() {
    BTree* car_trees[3];
    BTree* sp_trees[3];
    BTree* customer_trees[3];
    BTree* showroom_tree = init_btree(4);
    
    if (showroom_tree == NULL) {
        printf("Failed to initialize showroom tree\n");
        return 1;
    }

    for (int i = 0; i < 3; i++) {
        car_trees[i] = init_btree(1);
        sp_trees[i] = init_btree(2);
        customer_trees[i] = init_btree(3);
        
        if (car_trees[i] == NULL || sp_trees[i] == NULL || customer_trees[i] == NULL) {
            printf("Failed to initialize trees\n");
            for (int j = 0; j < i; j++) {
                free(car_trees[j]);
                free(sp_trees[j]);
                free(customer_trees[j]);
            }
            free(showroom_tree);
            return 1;
        }
    }

    if (access("showrooms.txt", F_OK) != -1) {
        load_from_file(showroom_tree, "showrooms.txt", 4);
    } else {
        printf("Warning: showrooms.txt not found\n");
    }

    char filename[50];
    for (int i = 0; i < 3; i++) {
        sprintf(filename, "showroom%d.txt", i + 1);
        if (access(filename, F_OK) != -1) {
            load_from_file(car_trees[i], filename, 1);
            load_from_file(sp_trees[i], filename, 2);
            load_from_file(customer_trees[i], filename, 3);
        } else {
            printf("Warning: %s not found\n", filename);
        }
    }

    int choice;
    do {
        printf("\n==== Car Showroom Management System ====\n");
        printf("1. Merge showrooms data and sort by VIN\n");
        printf("2. Add new sales person\n");
        printf("3. Find most popular car\n");
        printf("4. Find most successful sales person\n");
        printf("5. Sell car to customer\n");
        printf("6. Predict next month sales\n");
        printf("7. Display car information\n");
        printf("8. Search sales persons by sales range\n");
        printf("9. Print customers with EMI 36-48 months\n");
        printf("10. Add a new car\n");  
        printf("0. Exit\n");
        printf("Enter your choice: ");
        scanf("%d", &choice);

        switch (choice) {
            case 1: {
                merge_showrooms_sort_by_vin(car_trees, 3, "merged_cars.txt");
                break;
            }
            case 2: {
                SalesPerson* sp = (SalesPerson*)malloc(sizeof(SalesPerson));
                printf("Enter sales person ID: ");
                scanf("%d", &sp->id);
                printf("Enter sales person name: ");
                scanf(" %[^\n]", sp->name);
                printf("Enter showroom ID (1-3): ");
                scanf("%d", &sp->showroom_id);
                printf("Enter sales target (in lakhs): ");
                scanf("%f", &sp->target);
                sp->achieved = 0.0;
                sp->commission = 0.0;

                if (sp->showroom_id >= 1 && sp->showroom_id <= 3) {
                    add_sales_person(sp_trees[sp->showroom_id - 1], sp);

                    sprintf(filename, "showroom%d.txt", sp->showroom_id);
                    FILE* file = fopen(filename, "a");
                    if (file != NULL) {
                        fprintf(file, "%d,%s,%d,%.2f,%.2f,%.2f\n",
                            sp->id, sp->name, sp->showroom_id,
                            sp->target, sp->achieved, sp->commission);
                        fclose(file);
                    }
                } else {
                    printf("Invalid showroom ID\n");
                    free(sp);
                }
                break;
            }
            case 3: {
                find_most_popular_car(car_trees, 3);
                break;
            }
            case 4: {
                find_most_successful_sales_person(sp_trees, 3);
                break;
            }
            case 5: {
                char vin[MAX_VIN_LENGTH];
                printf("Enter car VIN: ");
                scanf(" %[^\n]", vin);

                Car* car = NULL;
                int showroom_id = -1;
                for (int i = 0; i < 3 && car == NULL; i++) {
                    car = (Car*)search_btree(car_trees[i]->root, vin);
                    if (car != NULL) {
                        showroom_id = i + 1;
                    }
                }

                if (car == NULL) {
                    printf("Car with VIN %s not found\n", vin);
                    break;
                }

                if (car->is_sold) {
                    printf("Car with VIN %s is already sold\n", vin);
                    break;
                }

                Customer* customer = (Customer*)malloc(sizeof(Customer));
                printf("Enter customer ID: ");
                scanf("%d", &customer->id);
                printf("Enter customer name: ");
                scanf(" %[^\n]", customer->name);
                printf("Enter customer mobile: ");
                scanf(" %[^\n]", customer->mobile);
                printf("Enter customer address: ");
                scanf(" %[^\n]", customer->address);
                printf("Enter car registration number: ");
                scanf(" %[^\n]", customer->registration_number);
                printf("Enter payment type (Cash/Loan): ");
                scanf(" %[^\n]", customer->payment_type);

                if (strcmp(customer->payment_type, "Loan") == 0) {
                    printf("Enter down payment (in lakhs): ");
                    scanf("%f", &customer->down_payment);

                    if (customer->down_payment < (car->price * 0.2)) {
                        printf("Down payment must be at least 20%% of car price (%.2f lakhs)\n", car->price * 0.2);
                        free(customer);
                        break;
                    }

                    printf("Enter EMI duration (36/60/84 months): ");
                    scanf("%d", &customer->emi_months);

                    if (customer->emi_months == 36) {
                        customer->interest_rate = 8.50;
                    } else if (customer->emi_months == 60) {
                        customer->interest_rate = 8.75;
                    } else if (customer->emi_months == 84) {
                        customer->interest_rate = 9.00;
                    } else {
                        printf("Invalid EMI duration. Choose 36, 60, or 84 months\n");
                        free(customer);
                        break;
                    }
                } else {
                    customer->down_payment = car->price;
                    customer->emi_months = 0;
                    customer->interest_rate = 0.0;
                }

                printf("Enter sales person ID: ");
                scanf("%d", &customer->sales_person_id);
                char sp_key[20];
                sprintf(sp_key, "%d", customer->sales_person_id);
                SalesPerson* sp = (SalesPerson*)search_btree(sp_trees[showroom_id - 1]->root, sp_key);

                if (sp == NULL) {
                    printf("Sales person with ID %d not found in showroom %d\n", customer->sales_person_id, showroom_id);
                    free(customer);
                    break;
                }

                sp->achieved += car->price;
                sp->commission = sp->achieved * 0.02;  

                FILE* sp_file = fopen(filename, "w");
                if (sp_file != NULL) {
                    save_salespeople_to_file(sp_trees[showroom_id - 1]->root, sp_file);
                    fclose(sp_file);
                }

                sell_car(car_trees[showroom_id - 1], customer_trees[showroom_id - 1], showroom_tree, vin, customer, customer->sales_person_id);

                sprintf(filename, "showroom%d.txt", showroom_id);
                FILE* car_file = fopen(filename, "w");
                if (car_file != NULL) {
                    save_cars_to_file(car_trees[showroom_id - 1]->root, car_file);
                    fclose(car_file);
                }

                sprintf(filename, "showroom%d.txt", showroom_id);
                FILE* cust_file = fopen(filename, "a");
                if (cust_file != NULL) {
                    fprintf(cust_file, "%d,%s,%s,%s,%s,%s,%s,%.2f,%d,%.2f,%d\n",
                        customer->id, customer->name, customer->mobile,
                        customer->address, customer->car_vin, customer->registration_number,
                        customer->payment_type, customer->down_payment,
                        customer->emi_months, customer->interest_rate, customer->sales_person_id);
                    fclose(cust_file);
                }

                FILE* showroom_file = fopen("showrooms.txt", "w");
                if (showroom_file != NULL) {
                    save_showrooms_to_file(showroom_tree->root, showroom_file);
                    fclose(showroom_file);
                }

                break;
            }
            case 6: {
                predict_next_month_sales(sp_trees, 3);
                break;
            }
            case 7: {
                char vin[MAX_VIN_LENGTH];
                printf("Enter car VIN: ");
                scanf(" %[^\n]", vin);

                bool found = false;
                for (int i = 0; i < 3 && !found; i++) {
                    Car* car = (Car*)search_btree(car_trees[i]->root, vin);
                    if (car != NULL) {
                        display_car_info(car_trees[i], vin);
                        found = true;
                    }
                }

                if (!found) {
                    printf("Car with VIN %s not found\n", vin);
                }
                break;
            }
            case 8: {
                float min_sales, max_sales;
                printf("Enter minimum sales (in lakhs): ");
                scanf("%f", &min_sales);
                printf("Enter maximum sales (in lakhs): ");
                scanf("%f", &max_sales);

                for (int i = 0; i < 3; i++) {
                    printf("\nShowroom %d:\n", i + 1);
                    search_sales_persons_by_range(sp_trees[i], min_sales, max_sales);
                }
                break;
            }
            case 9: {
                for (int i = 0; i < 3; i++) {
                    printf("\nShowroom %d:\n", i + 1);
                    print_customers_with_emi_range(customer_trees[i]);
                }
                break;
            }

            case 10: {
                int showroom_id;
                printf("Enter showroom ID to add car to (1-3): ");
                scanf("%d", &showroom_id);

                if (showroom_id >= 1 && showroom_id <= 3) {
                    char filename[20];
                    sprintf(filename, "showroom%d.txt", showroom_id);
                    add_car(car_trees[showroom_id-1], showroom_tree, filename);

                    // Update showrooms.txt
                    FILE* showroom_file = fopen("showrooms.txt", "w");
                    if (showroom_file != NULL) {
                        save_showrooms_to_file(showroom_tree->root, showroom_file);
                        fclose(showroom_file);
                    }
                } else {
                    printf("Invalid showroom ID\n");
                }
                break;
            }

            case 0:
                printf("Exiting program...\n");
                break;
            default:
                printf("Invalid choice. Please try again.\n");
        }
    } while (choice != 0);
    return 0;
}
