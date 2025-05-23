#include "core_filter_median.h"
// Forward declarations of sorting functions
static void bubble_sort(core_type_f64_t* arr, uint32_t n);
static void heap_sort(core_type_f64_t* arr, uint32_t n);
static void sift_down(core_type_f64_t* arr, uint32_t start, uint32_t end);

void core_filter_median_exec(
    const core_filter_median_inputs_t *i,
    core_filter_median_outputs_t *o,
    const core_filter_median_params_t *p,
    core_filter_median_state_t *state
)
{
    // Convenience pointers to vectors
    core_type_f64_t* acc = state->accumulator.vector;
    core_type_f64_t* sorted = state->sorted_accumulator.vector;
    const uint16_t size = p->selection_size;

    // Add new value to accumulator
    acc[state->head_index] = i->input;

    // Ensure curr_len is properly maintained
    if (state->accumulator.curr_len < size) {
        state->accumulator.curr_len++;
    }

    // Copy values to sorted accumulator for sorting
    for (uint16_t idx = 0; idx < state->accumulator.curr_len; idx++) {
        sorted[idx] = acc[idx];
    }
    state->sorted_accumulator.curr_len = state->accumulator.curr_len;

    // Sort using heap sort (more efficient) or bubble sort
    heap_sort(sorted, state->sorted_accumulator.curr_len);
    // bubble_sort(sorted, state->sorted_accumulator.curr_len);  // Alternative slower method

    // Calculate median
    if (state->accumulator.curr_len < size) {
        o->output = i->input;
    } else {
        if (size & 1) {
            o->output = sorted[size >> 1];
        } else {
            o->output = (sorted[(size >> 1) - 1] + sorted[size >> 1]) * 0.5;
        }
    }

    // Update head index for circular buffer
    state->head_index = (state->head_index + 1) % size;
}

static void bubble_sort(core_type_f64_t* arr, uint32_t n)
{
    for (uint32_t i = 0; i < n - 1; i++) {
        for (uint32_t j = 0; j < n - i - 1; j++) {
            if (arr[j] > arr[j + 1]) {
                core_type_f64_t temp = arr[j];
                arr[j] = arr[j + 1];
                arr[j + 1] = temp;
            }
        }
    }
}

static void sift_down(core_type_f64_t* arr, uint32_t start, uint32_t end)
{
    uint32_t root = start;

    while ((root << 1) + 1 <= end) {
        uint32_t child = (root << 1) + 1;
        uint32_t swap = root;

        if (arr[swap] < arr[child]) {
            swap = child;
        }
        if (child + 1 <= end && arr[swap] < arr[child + 1]) {
            swap = child + 1;
        }
        if (swap == root) {
            return;
        }

        core_type_f64_t temp = arr[root];
        arr[root] = arr[swap];
        arr[swap] = temp;
        root = swap;
    }
}

static void heap_sort(core_type_f64_t* arr, uint32_t n)
{
    if (n < 2) return;

    // Build heap (rearrange array)
    int32_t start = (n - 2) >> 1; // Last non-leaf node
    while (start >= 0) {
        sift_down(arr, start, n - 1);
        start--;
    }

    // Extract elements from heap one by one
    uint32_t end = n - 1;
    while (end > 0) {
        // Move current root to end
        core_type_f64_t temp = arr[end];
        arr[end] = arr[0];
        arr[0] = temp;

        // Call sift_down on reduced heap
        end--;
        sift_down(arr, 0, end);
    }
}