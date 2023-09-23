#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "heapsort.h"
#include "customer.h"

#define CUSTOMER_SIZE sizeof(customer)

/* helper: returns the max of a heap entry and it's two direct children
 * * returns NULL if failure occured, and the maximum otherwise
 */
customer *get_max(customer *curr, customer *children[2], size_t curr_index, size_t num_records, FILE *file) {
    customer *max = curr;
    for (int i = 0; i <= 1; i++)
    {
        int child_index = 2 * curr_index + (i + 1);
        if (child_index < num_records)
        {
            if (fseek(file, child_index * CUSTOMER_SIZE, SEEK_SET)) return NULL;
            if (fread(children[i], CUSTOMER_SIZE, 1, file) < 1) return NULL;

            if (children[i]->loyalty > max->loyalty || (children[i]->loyalty == max->loyalty && strncmp(children[i]->name, max->name, CUSTOMER_NAME_MAX) > 0))
            {
                max = children[i];
            }
        }
    }
    return max;
}

/* helper: heapify max heap at index index 
 * returns 0 if failure occured, 1 otherwise
 */
int heapify(size_t index, size_t num_records, FILE *file)
{
    int swap_index;
    
    // get current node
    if (fseek(file, index * CUSTOMER_SIZE, SEEK_SET)) return 0;
    customer *curr = malloc(CUSTOMER_SIZE);
    if (fread(curr, CUSTOMER_SIZE, 1, file) < 1) return 0;

    // find max of current node and its children
    customer *children[2];
    children[0] = malloc(CUSTOMER_SIZE);
    children[1] = malloc(CUSTOMER_SIZE);
    customer *max = get_max(curr, children, index, num_records, file);

    while (max != curr)
    {
        if (max == NULL) return 0;
        // set index of child to be swapped
        if (max == children[0]) swap_index = 2 * index + 1;
        else swap_index = 2 * index + 2;

        // swap current and child
        if (fseek(file, swap_index * CUSTOMER_SIZE, SEEK_SET)) return 0;
        fwrite(curr, CUSTOMER_SIZE, 1, file);
        if (fseek(file, index * CUSTOMER_SIZE, SEEK_SET)) return 0;
        fwrite(max, CUSTOMER_SIZE, 1, file);

        // set new curr index
        index = swap_index;
        if (fseek(file, index * CUSTOMER_SIZE, SEEK_SET)) return 0;
        if (fread(curr, CUSTOMER_SIZE, 1, file) < 1) return 0;

        // find max of current node and its children
        max = get_max(curr, children, index, num_records, file);
    }

    free(curr);
    free(children[0]);
    free(children[1]);

    return 0;
}

int heapsort(const char *filename)
{

    // attempt to open file, returning 1 if error occurs
    FILE *file = fopen(filename, "rb+");
    if (file == NULL) return 0;

    // get number of records
    if (fseek(file, 0L, SEEK_END)) return 0;
    int num_records = ftell(file) / CUSTOMER_SIZE;

    // set up max heap
    for (int i = (num_records / 2) - 1; i >= 0; i--)
    {
        heapify(i, num_records, file);
    }

    // for extracting max
    customer *max = malloc(CUSTOMER_SIZE);
    customer *bubble = malloc(CUSTOMER_SIZE);

    // repeatedly extract max and heapify to sort array
    for (int i = num_records - 1; i >= 0; i--)
    {
        // get max
        if (fseek(file, 0L, SEEK_SET)) return 0;
        if (fread(max, CUSTOMER_SIZE, 1, file) < 1) return 0;

        // replace max with lowest entry of heap
        if (fseek(file, i * CUSTOMER_SIZE, SEEK_SET)) return 0;
        if (fread(bubble, CUSTOMER_SIZE, 1, file) < 1) return 0;
        if (fseek(file, 0L, 0)) return 0;
        fwrite(bubble, CUSTOMER_SIZE, 1, file);

        // use heapify to "bubble" entry to correct spot
        heapify(0, i + 1, file);

        // place the max at the "emptied" spot
        if (fseek(file, i * CUSTOMER_SIZE, SEEK_SET)) return 0;
        fwrite(max, CUSTOMER_SIZE, 1, file);
    }

    // close file and free allocated heap memory
    fclose(file);
    free(max);
    free(bubble);

    return 1;
}
