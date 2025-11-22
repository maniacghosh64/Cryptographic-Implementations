#include <stdbool.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include<math.h>

typedef void (*SortFunction)(int*, int, long long*, long long*);

int compare_ll(const void *a, const void *b) {
    long long val1 = *(long long *)a;
    long long val2 = *(long long *)b;
    return (val1 > val2) - (val1 < val2);  // Safe comparison
}

double get_median(long long *arr, int n) {
    qsort(arr, n, sizeof(long long), compare_ll);
    if (n % 2 == 0)
        return (arr[n/2 - 1] + arr[n/2]) / 2.0;
    else
        return arr[n/2];
}


void benchmarkSort(
    const char* sort_name,
    SortFunction sort_func,
    int size,
    int runs,
    double complexity,
    FILE* fp
) {
    int *arr = malloc(size * sizeof(int));
    long long *run_comparisons = malloc(runs * sizeof(long long));
    long long *run_swaps = malloc(runs * sizeof(long long));

    long long total_comparisons = 0;
    long long total_swaps = 0;

    clock_t start = clock();
    for (int r = 0; r < runs; r++) {
        for (int i = 0; i < size; i++) {
            arr[i] = (rand() % 100) + 1;
        }

        long long comps = 0, swps = 0;
        sort_func(arr, size, &comps, &swps);

        run_comparisons[r] = comps;
        run_swaps[r] = swps;

        total_comparisons += comps;
        total_swaps += swps;
    }
    clock_t end = clock();

    double median_comps = get_median(run_comparisons, runs);
    double median_swaps = get_median(run_swaps, runs);
    double const_sort = (double)total_comparisons / complexity;

    long long min_comps = run_comparisons[0], max_comps = run_comparisons[0];
    long long min_swaps = run_swaps[0], max_swaps = run_swaps[0];
    for (int i = 1; i < runs; i++) {
        if (run_comparisons[i] < min_comps) min_comps = run_comparisons[i];
        if (run_comparisons[i] > max_comps) max_comps = run_comparisons[i];
        if (run_swaps[i] < min_swaps) min_swaps = run_swaps[i];
        if (run_swaps[i] > max_swaps) max_swaps = run_swaps[i];
    }

    double cpu_time = (double)(end - start) / CLOCKS_PER_SEC;

    printf("\n--- %s ---\n", sort_name);
    printf("Array size: %d, Runs: %d\n", size, runs);
    printf("CPU time: %.4f seconds\n", cpu_time);
    printf("Avg comparisons: %.2f\n", (double)total_comparisons / runs);
    printf("Min comparisons: %lld, Max comparisons: %lld, Median: %.2f\n", min_comps, max_comps, median_comps);
    printf("Dividing number of comparisons by complexity: %.2f\n", const_sort);
    printf("Avg swaps: %.2f\n", (double)total_swaps / runs);
    printf("Min swaps: %lld, Max swaps: %lld, Median: %.2f\n", min_swaps, max_swaps, median_swaps);

    double clock_speed_hz = 3e9;  // CPU frequency, used here as a placeholder
    double clock_cycles = cpu_time * clock_speed_hz;

    fprintf(fp, "%s,%d,%d,%.6f,%.0f,%.2f,%.2f,%.2f\n",
    sort_name, size, runs, cpu_time, clock_cycles,
    (double)total_comparisons / runs,
    (double)total_swaps / runs,
    const_sort);

    free(arr);
    free(run_comparisons);
    free(run_swaps);
}


void swap_bubble(int* x, int* y){
    int temp = *x;
    *x = *y;
    *y = temp;
}

void bubbleSort(int arr[], int n, long long *comparisons, long long *swaps){
    int i, j;
    bool swapped;
    for (i = 0; i < n - 1; i++) {
        swapped = false;
        for (j = 0; j < n - i - 1; j++) {
            (*comparisons)++;
            if (arr[j] > arr[j + 1]) {
                swap_bubble(&arr[j], &arr[j + 1]);
                swapped = true;
                (*swaps)++;
            }
        }
        // If no two elements were swapped by inner loop,
        // then break, indicating that the array is already sorted
        if (swapped == false)
            break;
    }
}


void heapify(int arr[], int n, int i, long long *comparisons, long long *swaps)
{
    int maximum = i;
    int left_index = 2 * i + 1;
    int right_index = 2 * i + 2;

    // Compare with left child
    if (left_index < n) {
        (*comparisons)++;
        if (arr[left_index] > arr[maximum])
            maximum = left_index;
    }

    // Compare with right child
    if (right_index < n) {
        (*comparisons)++;
        if (arr[right_index] > arr[maximum])
            maximum = right_index;
    }

    // Swap if needed
    if (maximum != i) {
        int temp = arr[i];
        arr[i] = arr[maximum];
        arr[maximum] = temp;
        (*swaps)++;
        heapify(arr, n, maximum, comparisons, swaps); // recursive call
    }
}

void heapSort(int arr[], int n, long long *comparisons, long long *swaps)
{
    // Build heap (rearrange array)
    for (int i = n / 2 - 1; i >= 0; i--)
        heapify(arr, n, i, comparisons, swaps);

    // Extract elements one by one from heap
    for (int i = n - 1; i > 0; i--) {
        // Move current root to end
        int temp = arr[0];
        arr[0] = arr[i];
        arr[i] = temp;
        (*swaps)++;

        // call max heapify on the reduced heap
        heapify(arr, i, 0, comparisons, swaps);
    }
}


void merge(int arr[], int l, int m, int r, long long *comparisons, long long *swaps) {
    int i, j, k;
    int n1 = m - l + 1;
    int n2 = r - m;

    int* L = (int*)malloc(n1 * sizeof(int));
    int* R = (int*)malloc(n2 * sizeof(int));

    if (L == NULL || R == NULL) {
        return;
    }

    for (i = 0; i < n1; i++)
        L[i] = arr[l + i];
    for (j = 0; j < n2; j++)
        R[j] = arr[m + 1 + j];

    i = 0; j = 0; k = l;

    while (i < n1 && j < n2) {
        (*comparisons)++;
        if (L[i] <= R[j]) {
            arr[k] = L[i];
            i++;
        } else {
            arr[k] = R[j];
            j++;
        }
        k++;
        (*swaps)++;  // Each assignment to arr[k] is a move
    }

    while (i < n1) {
        arr[k++] = L[i++];
        (*swaps)++;
    }

    while (j < n2) {
        arr[k++] = R[j++];
        (*swaps)++;
    }

    free(L);
    free(R);
}

void mergeSortInstrumented(int arr[], int l, int r, long long *comparisons, long long *swaps) {
    if (l < r) {
        int m = l + (r - l) / 2;
        mergeSortInstrumented(arr, l, m, comparisons, swaps);
        mergeSortInstrumented(arr, m + 1, r, comparisons, swaps);
        merge(arr, l, m, r, comparisons, swaps);
    }
}

void mergeSort(int arr[], int n, long long *comparisons, long long *swaps) {
    mergeSortInstrumented(arr, 0, n - 1, comparisons, swaps);
}


void swap_quick(int* a, int* b, long long *swaps) {
    int temp = *a;
    *a = *b;
    *b = temp;
    (*swaps)++;
}

// Partition function with counting
int partition(int arr[], int low, int high, long long *comparisons, long long *swaps) {
    int pivot = arr[high];
    int i = low - 1;

    for (int j = low; j <= high - 1; j++) {
        (*comparisons)++;
        if (arr[j] < pivot) {
            i++;
            swap_quick(&arr[i], &arr[j], swaps);
        }
    }

    swap_quick(&arr[i + 1], &arr[high], swaps);
    return i + 1;
}

// Recursive QuickSort with counting
void quickSortInstrumented(int arr[], int low, int high, long long *comparisons, long long *swaps) {
    if (low < high) {
        int pi = partition(arr, low, high, comparisons, swaps);
        quickSortInstrumented(arr, low, pi - 1, comparisons, swaps);
        quickSortInstrumented(arr, pi + 1, high, comparisons, swaps);
    }
}

// Wrapper to match benchmarkSort() signature
void quickSort(int arr[], int n, long long *comparisons, long long *swaps) {
    quickSortInstrumented(arr, 0, n - 1, comparisons, swaps);
}

int main() {
    srand(time(NULL));
    int runs = 10000;

    FILE *fp = fopen("benchmark_results.csv", "w");
    if (fp == NULL) {
        printf("Error opening file for writing. \n");
        return 1;
    }
    fprintf(fp, "sort,size,runs,cpu_time,clock_cycles,avg_comps,avg_swaps,constant_value\n");
    for (int size = 100; size <= 1000; size += 100) {
        benchmarkSort("Bubble Sort", bubbleSort, size, runs, pow((double)size, 2), fp);
        benchmarkSort("Quick Sort", quickSort, size, runs, (double)size * log((double)size),  fp);
        benchmarkSort("Merge Sort", mergeSort, size, runs, (double)size * log((double)size), fp);
        benchmarkSort("Heap Sort", heapSort, size, runs, (double)size * log((double)size), fp);
    }
    fclose(fp);
    return 0;
}
