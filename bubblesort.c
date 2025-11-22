#include <stdbool.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>

void swap(int* xp, int* yp){
    int temp = *xp;
    *xp = *yp;
    *yp = temp;
}

// An optimized version of Bubble Sort
void bubbleSort(int arr[], int n){
    int i, j;
    bool swapped;
    for (i = 0; i < n - 1; i++) {
        swapped = false;
        for (j = 0; j < n - i - 1; j++) {
            if (arr[j] > arr[j + 1]) {
                swap(&arr[j], &arr[j + 1]);
                swapped = true;
            }
        }

        // If no two elements were swapped by inner loop,
        // then break, indicating that the array is already sorted
        if (swapped == false)
            break;
    }
}

// Function to print an array
void printArray(int arr[], int size){
    int i;
    for (i = 0; i < size; i++)
        printf("%d ", arr[i]);
}

int main(){
    // Seeding the random number generator with current time 
    srand(time(NULL));
    int arr1[100];
    int arr2[200];
    int arr3[300];
    int arr4[400];
    int arr5[500];
    int arr6[600];
    int arr7[700];
    int arr8[800];
    int arr9[900];
    int arr10[1000];
    // will need this later: int n = sizeof(arr) / sizeof(arr[0]);


    clock_t start = clock();
    for (int j = 0; j < 10000; j++) {
        
        for (int i = 0; i < 100; i++) {
            
            int random_number = (rand() % 100) + 1;
            arr1[i] = random_number;
        }
        bubbleSort(arr1, 100);
    }
    clock_t end = clock();
    double cpu_time_used = (double)(end-start) / CLOCKS_PER_SEC;
    printf("\nCPU time used: %f seconds for 100 length array", cpu_time_used);


    start = clock();
    for (int j = 0; j < 10000; j++) {
        for (int i = 0; i < 200; i++) {
            int random_number = (rand() % 100) + 1;
            arr2[i] = random_number;
        }        
        bubbleSort(arr2, 200);
    }
    end = clock();
    cpu_time_used = (double)(end-start) / CLOCKS_PER_SEC;
    printf("\nCPU time used: %f seconds for 200 length array", cpu_time_used);


    start = clock();
    for (int j = 0; j < 10000; j++) {
        for (int i = 0; i < 300; i++) {
            int random_number = (rand() % 100) + 1;
            arr3[i] = random_number;
        }
        
        bubbleSort(arr3, 300);
    }
    end = clock();
    cpu_time_used = (double)(end-start) / CLOCKS_PER_SEC;
    printf("\nCPU time used: %f seconds for 300 length array", cpu_time_used);


    start = clock();
    for (int j = 0; j < 10000; j++) {
        for (int i = 0; i < 400; i++) {
            int random_number = (rand() % 100) + 1;
            arr4[i] = random_number;
        }
        bubbleSort(arr4, 400);
    }
    end = clock();
    cpu_time_used = (double)(end-start) / CLOCKS_PER_SEC;
    printf("\nCPU time used: %f seconds for 400 length array", cpu_time_used);

    
    start = clock();
    for (int j = 0; j < 10000; j++) {
        for (int i = 0; i < 500; i++) {
            int random_number = (rand() % 100) + 1;
            arr5[i] = random_number;
        }
        bubbleSort(arr5, 500);
    }
    end = clock();
    cpu_time_used = (double)(end-start) / CLOCKS_PER_SEC;
    printf("\nCPU time used: %f seconds for 500 length array", cpu_time_used);

    
    start = clock();
    for (int j = 0; j < 10000; j++) {
        for (int i = 0; i < 600; i++) {
            int random_number = (rand() % 100) + 1;
            arr6[i] = random_number;
        }
        bubbleSort(arr6, 600);
    }
    end = clock();
    cpu_time_used = (double)(end-start) / CLOCKS_PER_SEC;
    printf("\nCPU time used: %f seconds for 600 length array", cpu_time_used);

    
    start = clock();
    for (int j = 0; j < 10000; j++) {
        for (int i = 0; i < 700; i++) {
            int random_number = (rand() % 100) + 1;
            arr7[i] = random_number;
        }
        bubbleSort(arr7, 700);
    }
    end = clock();
    cpu_time_used = (double)(end-start) / CLOCKS_PER_SEC;
    printf("\nCPU time used: %f seconds for 700 length array", cpu_time_used);

    
    start = clock();
    for (int j = 0; j < 10000; j++) {
        for (int i = 0; i < 800; i++) {
            int random_number = (rand() % 100) + 1;
            arr8[i] = random_number;
        }
        bubbleSort(arr8, 800);
    }
    end = clock();
    cpu_time_used = (double)(end-start) / CLOCKS_PER_SEC;
    printf("\nCPU time used: %f seconds for 800 length array", cpu_time_used);

    
    start = clock();
    for (int j = 0; j < 10000; j++) {
        for (int i = 0; i < 900; i++) {
            int random_number = (rand() % 100) + 1;
            arr9[i] = random_number;
        }
        bubbleSort(arr9, 900);
    }
    end = clock();
    cpu_time_used = (double)(end-start) / CLOCKS_PER_SEC;
    printf("\nCPU time used: %f seconds for 900 length array", cpu_time_used);

    
    start = clock();
    for (int j = 0; j < 10000; j++) {
        for (int i = 0; i < 1000; i++) {
            int random_number = (rand() % 100) + 1;
            arr10[i] = random_number;
        }
        bubbleSort(arr10, 1000);
    }
    end = clock();
    cpu_time_used = (double)(end-start) / CLOCKS_PER_SEC;
    printf("\nCPU time used: %f seconds for 1000 length array", cpu_time_used);


    return 0;
}
