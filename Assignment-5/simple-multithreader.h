#include <iostream>
#include <list>
#include <functional>
#include <stdlib.h>
#include <cstring>
#include <sys/time.h>
using namespace std;


//Function Declarations to avoid any kind of precedence order issues.
void *to_be_executed_by_thread_matrix(void* ptr);
void *to_be_executed_by_thread_vector(void *ptr);
void parallel_for(int start, int end,function<void(int)> &&lambda, int no_of_threads);
void parallel_for(int start_1, int end_1, int start_2, int end_2,function<void(int, int)> &&lambda, int no_of_threads);

//Below We are defining structure to have start and ending index for threads..

typedef struct { // for runinng 2d
  function<void(int,int)> lambda; //Lambda function that is given to us in matrix.cpp ..
  int start_1; //Outside loop to start from this index.
  int end_1; //Outside loop to end at this index but excluding this index.
  pthread_t tid; // Thread id for thread..
  int start_2; //Inside loop start.
  int end_2; //Inside loop ending (not inclusive)...
}threads_matrix;

typedef struct{
  function<void(int)>lambda; //Lambda function to be given inside vector.cpp file...
  pthread_t tid; // Thread id for thread..
  int start; //Loop starting index.
  int end; //Loop ennding+1 index.
} threads_vector;




void parallel_for(int start, int end,function<void(int)> &&lambda, int no_of_threads){ //We are calling this in code
//This has to provide abstraction to code, assuming threads getting handled properly
if(end<=start || no_of_threads <=0){
  cout <<"Inapropriate calling of function by User !!"<<endl;
  exit(1);
}
//If Checks get Passed..
struct timeval start_time, end_time;
gettimeofday(&start_time, NULL); //Storing Starting time for process to get started.. (All Threads..)
no_of_threads-=1;
//Assuming the main thread is amoung one of the threads, so subtracted one..
threads_vector args[no_of_threads]; //To store thread data, custom data structure prepared by us..

int size_indexes=(end-start); // Calculed Size needed..
int chunk_size = (size_indexes)/no_of_threads; // Whole chunks needed, to complete this execution
int starting_incase_not_completed=-1;
if((size_indexes)%no_of_threads!=0){ //if not all indexes are covered, to cover last case where size for that chuck will be of less than others..
  starting_incase_not_completed=chunk_size*no_of_threads; //Starting index for that leaving range [this_ponit,end)
}
for (int i=0; i<no_of_threads;i++){
  args[i].start=i*chunk_size; // storing starting index for this particalar thread which is just being created..
  args[i].end=(i+1)*chunk_size; //Simillarly storing ending index..
  args[i].lambda=lambda; // Just passing lambda function given by user, which is just normal cal. to be performed on every index..
  if (pthread_create(&args[i].tid,NULL,to_be_executed_by_thread_vector,(void*) &args[i])!=0){ //Creating thread
      cout <<i<<" th Thread Creating Failed\n";
      cout<<"Aborting Process\n";
      exit(1);
  }
}
threads_vector rem_th;
if(starting_incase_not_completed!=-1){
  // This means chunks are not evenly divided and we need to create a more thread execute remaining indexes.
  rem_th.end=end; 
  rem_th.start=starting_incase_not_completed;
  rem_th.lambda=lambda;
  if (pthread_create(&rem_th.tid, NULL, to_be_executed_by_thread_vector, (void*)&rem_th)!=0){
      cout <<"Remaing thread craetion failed\n";
      exit(1);
  }
}

for(int i=0;i<no_of_threads;i++){
  pthread_join(args[i].tid,NULL); //Joining all Threads 
}
if(starting_incase_not_completed!=-1){
  pthread_join(rem_th.tid, NULL); //Incase we have additional thread join that too...
}
gettimeofday(&end_time, NULL);  //storing ending time
double execution_time =(end_time.tv_sec - start_time.tv_sec)+((end_time.tv_usec - start_time.tv_usec)/double(1000000));
// Calculating execution time..
cout<<"Execution Time Using Threading (1-D) : "<<execution_time<<" second"<<endl;
}


// We are Overriding this function for matrices..
void parallel_for(int start_1, int end_1, int start_2, int end_2,function<void(int, int)> &&lambda, int no_of_threads){
struct timeval start_time, end_time;
gettimeofday(&start_time, NULL);
//Noting Down time for starting of process.

no_of_threads-=1;
//As main function is amoung one of the threads..
threads_matrix args[no_of_threads];
//Creating an array of arguments for threads being generated..
int size_indexes1,size_indexes2,size_of_chunk_upperLoop,size_of_chunk_lowerLoop;
size_indexes1=end_1-start_1;
size_indexes2= end_2-start_2;

size_of_chunk_lowerLoop =(size_indexes2)/no_of_threads;
size_of_chunk_upperLoop=(size_indexes1)/no_of_threads;

int starting_incase_not_completed=-1;
if(size_indexes1%no_of_threads!=0)  starting_incase_not_completed=(size_of_chunk_upperLoop*no_of_threads); 

for(int i=0;i<no_of_threads;i++){
  //Storing Sufficient arguments..
  args[i].start_1=i*size_of_chunk_upperLoop;
  args[i].end_1=(i+1)*size_of_chunk_upperLoop;
  args[i].start_2=start_2;
  args[i].end_2=end_2;
  args[i].lambda=lambda;
  if (pthread_create(&args[i].tid, NULL, to_be_executed_by_thread_matrix,(void*) &args[i])!=0) {
    cout <<i<<" th Thread Creating Failed\n";
    cout<<"Aborting Process\n";
    exit(1);
  }
}
threads_matrix rem_th; //Remaining Thread...

if(starting_incase_not_completed!=-1){
rem_th.end_1=end_1;
rem_th.start_1=starting_incase_not_completed;
rem_th.lambda=lambda;
rem_th.end_2 = end_2;
rem_th.start_2 = start_2;
if(pthread_create(&rem_th.tid, NULL, to_be_executed_by_thread_matrix,(void*) &rem_th)!=0){
  cout<<"Remaining Thread Creation failed\nAborting Process\n";
  exit(1);
}
}

//Thread Joining Code from Sir Class Lecture Slides..
for(int i=0;i<no_of_threads;i++) {
  if(pthread_join(args[i].tid, NULL)!=0){
    cout<<"Joining Operation Failed\n";
    exit(1);
  }
}
//Joing for Left Over Thread..
if(starting_incase_not_completed!=-1){
  if(pthread_join(rem_th.tid,NULL)!=0){
    cout<<"Joining Op. Failed !!"<<endl;
    exit(1);
  }
}
// As soon as Joining Completes Save time..
gettimeofday(&end_time, NULL);
double execution_time=(end_time.tv_sec-start_time.tv_sec)+ ((end_time.tv_usec-start_time.tv_usec)/double(1000000));
cout<<"Execution Time Using Threading (2-D) : "<<execution_time<<" second"<<endl;
}

// These function will be executed for each threads
void *to_be_executed_by_thread_matrix(void* ptr){
if (ptr==nullptr){
  cout<<"Thread Pointer is found null, So terminating the code to prevent not desired behaviour"<<endl;
  exit(1);
}
else{
  //Storing Sufficient arguments..
  threads_matrix *thread=(threads_matrix*) ptr; //Typecasting void ptr to thread ptr..
  int lower_loop_st=thread->start_2;
  int lower_loop_en=thread->end_2;
  int upper_loop_st=thread->start_1;
  int upper_loop_en=thread->end_1;
  for(int i=upper_loop_st; i<upper_loop_en;i++){ //Just following to every possible index, 
    for(int j=lower_loop_st;j<lower_loop_en;j++){
      thread->lambda(i,j); // Applying lamda function give by matrix.cpp to index pairs..
      // Calling there lambda function for each pair.
    }
  }
}
  return nullptr;
}

void *to_be_executed_by_thread_vector(void *ptr){
if(ptr==nullptr){
  cout<<"Thread Pointer is found null, So terminating the code to prevent not desired behaviour"<<endl;
  exit(1);
}
else{
  threads_vector* thread=((threads_vector*) ptr); //Typecasting void ptr to thread ptr..
  int starting_index=thread->start; //getting starting index..
  int ending_index=thread->end; //getting ending index.. (again not inclusive)
  for(int i=starting_index; i<ending_index; i++){ // iterating over these indexes and appling lamda functions
    thread->lambda(i); //application of lamda function..
  }
}
  return nullptr;
}


int user_main(int argc, char **argv);
int main(int argc, char **argv) {
    cout << "====== Welcome to Assignment-5 of the CSE231(A) ======\n";
    int x = user_main(argc, argv);
    cout << "====== Hope you enjoyed CSE231(A) ======\n";
    return x;
}
#define main user_main

