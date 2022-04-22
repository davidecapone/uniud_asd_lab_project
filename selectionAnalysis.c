#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <limits.h>
#include <string.h>
#include "implementations/quickSelect.c"
#include "implementations/heapSelect.c"
#include "implementations/medianMediansSelect.c"

#define MALLOC_ARRAY(number, type)\
	((type*) malloc((number) *sizeof(type)))

const double E = 0.001;
double Tmin;

int A = 100;
double B = 0.157673137;

typedef enum {
  True,
  False
} ArrayOrdered;

typedef enum {
	QuickSelect,
	HeapSelect,
	MedianMediansSelect
} Algorithm;

typedef enum {
  square_n,     // k = sqrt(n)
  divided_n,    // k = n/2
  random_k,
  quickselect_worstcase   // k = 0
} Analysis;

typedef enum {
  testing,
  release
} VControl;

double duration(struct timespec start, struct timespec end) {
	return end.tv_sec - start.tv_sec +
		((end.tv_nsec - start.tv_nsec) / (double) 1000000000.0);
}

double getResolution() {
	struct timespec start, end;
	clock_gettime(CLOCK_MONOTONIC, &start);
	do {
		clock_gettime(CLOCK_MONOTONIC, &end);
	} while (duration(start, end) == 0.0);
	return duration(start, end);
}

/**
 * @brief Ottenere il tempo minimo misurabile dalla macchina in base al'errore minimo ammissibile
 * 
 * @return double : tempo minimo misurabile in secondi
 */
double get_t_min() {
	double R = getResolution();
	return (R*(1/E+1));
}

/**
 * @brief Generare dimensioni per array che crescono esponenzialmente
 * 
 * @param i numero da usare come esponente
 * @return int : dimensione 
 */
int exp_distribution(int i) {
	return ( A * pow(2, B*i) );
}

/**
 * @brief Generare numeri interi pseudocasuali
 * 
 * @return int : numero compreso tra -INT_MAX e +INT_MAX
 */
int get_random_int() {
  int randomUnsigned = rand();
  int sign = rand() % 2;
  return ( sign == 0 ) ? ( -1 * randomUnsigned ) : randomUnsigned;
}

/**
 * @brief Popola l'array di interi
 * 
 * @param A array da popolare
 * @param size dimensione array
 * @param order tipo di riempimento (True: ordinato, False: random)
 */
void populate( int A[], int size, ArrayOrdered order ) {
  if ( order == True ) {
    for (int i = 0; i < size; i++) A[i] = i;
  } else {
    for (int i = 0; i < size; i++) A[i] = get_random_int();
  }
}

/**
 * @brief Calcola tempo di esecuzione di un campione A di interi
 * 
 * @param type quale algoritmo utilizzare
 * @param A campione
 * @param size dimensione array
 * @param k posizione da determinare
 * @return int : tempo di esecuzione (in secondi)
 */
double get_execution_time( Algorithm type, int A[], int size, int k ) {
  struct timespec start, end;
  struct timespec backup_start, backup_end;
  double period = 0.0;
  double backupTime = 0.0;

  // n.ro di iterazioni (misurazioni) effettuate sul campione:
  int count = 0;

  // backup del campione in 'copy':
  int *copy = NULL;
  copy = MALLOC_ARRAY(size, int);
  memcpy(copy, A, size*sizeof(int));

  int kSmallest;
	clock_gettime(CLOCK_MONOTONIC, &start);
	do {
		switch (type) {
			case QuickSelect:
				kSmallest = quickSelect(A, 0, size-1, k);
				break;
			case HeapSelect:
				kSmallest = heapSelect(A, 0, size-1, k);
				break;
			case MedianMediansSelect:
				kSmallest = MoMSelect(A, 0, size-1, k);
				break;
      default:
        break;
		}

		clock_gettime(CLOCK_MONOTONIC, &end);
		period = duration(start, end);

    // A sarà la vecchia istanza di A (vecchia copia):
    if (period <= Tmin) {
      clock_gettime(CLOCK_MONOTONIC, &backup_start);
      memcpy(A, copy, size*sizeof(int));
      clock_gettime(CLOCK_MONOTONIC, &backup_end);

      // tempo di backup da sottrarre al termine:
      backupTime += duration(backup_start, backup_end);
    }
    
		count++;
	} while (period <= Tmin);

  free(copy);
	return ((double) ((period - backupTime) / count));
}

/**
 * @brief Compila il file con i dati 
 * 
 * @param analysisType tipo di analisi
 * @param size dimensione degli array
 * @param n_samples numero di array
 * @param k posizione da determinare
 * @param ptr puntatore al file
 */
void execute_samples( Analysis type, int size, int n_samples, ArrayOrdered order, FILE * ptr ) {
	int *sample = NULL;
  double quickSelectTime = 0;
  double heapSelectTime = 0;
  double medianSelectTime = 0;

  int k;
  // quale k scegliere?
  if (type == quickselect_worstcase) k = 0;
  else if (type == divided_n) k = size / 2;
  else if (type == square_n) k = sqrt(size);

  // creare 100 campioni dimensione size, calcolare tempo medio esecuzione
  for (int i = 1; i <= n_samples; i++) {

    sample = MALLOC_ARRAY(size, int);
    populate(sample, size, order);

    if (type == random_k) k = rand() % size;

    if (type == quickselect_worstcase) {
      quickSelectTime = get_execution_time( QuickSelect, sample, size, k );
      fprintf(ptr, "%d, %f\n", size, quickSelectTime);
      printf("SIZE: %d, quickSelect: %f\n", size, quickSelectTime);
    } else {
      quickSelectTime = get_execution_time( QuickSelect, sample, size, k );
      heapSelectTime = get_execution_time( HeapSelect, sample, size, k );
      medianSelectTime = get_execution_time( MedianMediansSelect, sample, size, k );
      fprintf(ptr, "%d, %f, %f, %f\n", size, quickSelectTime, heapSelectTime, medianSelectTime);
      printf("SIZE: %d, quickSelect: %f - heapSelect: %f - medianMediansSelect: %f \n", size, quickSelectTime, heapSelectTime, medianSelectTime);
    }

    free(sample);
  }
}

FILE * setup_csv ( Analysis type ) {
  char filename[50];
  FILE * ptr;
  switch (type) {
    case square_n:
      strcpy(filename, "dataset/square_n.csv");
      break;
    case divided_n:
      strcpy(filename, "dataset/divided_n.csv");
      break;
    case random_k:
      strcpy(filename, "dataset/random_k.csv");
      break;
    case quickselect_worstcase:
      strcpy(filename, "dataset/quickselect_worst.csv");
      break;
    default:
      break;
  }

  ptr = fopen(filename, "w");
  if (type == quickselect_worstcase) fprintf(ptr, "size,quickselect\n");
  else fprintf(ptr, "size,quickselect,heapselect,medianmediansselect\n");
  fclose(ptr);
  ptr = fopen(filename, "a");
  return ptr;
}

/**
 * @brief esecuzione dell'analisi su n_samples campioni per ogni dimensione
 * 
 * @param type tipo di analisi da effettuare
 * @param n_samples quanti campioni generare per ogni dimensione exp.
 */
void analysis( Analysis type, int n_samples ) {
  // dimensione finale (default) da raggiungere 5 milioni
  int size, k, final = 99;

  FILE * ptr = setup_csv(type);
  ArrayOrdered order = False ;

  if (type == quickselect_worstcase) {
    order = True;
    // riduciamo la dimensione massima da raggiungere a 7097:
    final = 39;
  }

  for (int i = 0; i <= final; i++) {
    // dimensione exp. dei campioni
    size = exp_distribution( i );
    execute_samples( type, size, n_samples, order, ptr );
  }

  fclose(ptr);
}

int main () {
  srand(time(NULL));
  Tmin = get_t_min();
  int n_samples = 20;
  printf("\e[1;1H\e[2J");

  // analisi k = sqrt(n)
  //analysis(square_n, 20);

  // analisi k = n/2
  //analysis(divided_n, 20);

  // aumentiamo la numerosità dei campioni per evidenziare maggiormente la varianza:
  //analysis(random_k, 35);

  // a partire da test effettuati si evidenzia maggiormente l'andamento con 50 campioni per dimensione:
  analysis(quickselect_worstcase, 50);

  return (EXIT_SUCCESS);
}