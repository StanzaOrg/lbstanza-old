#include<pthread.h>
#include<stdio.h>
#include<stdlib.h>
#include<stanza.h>

//============================================================
//=================== Explanation of Stop ====================
//============================================================
//
//What the user should expect:
//  1) Call stop.
//  2) Retrieve the contents.
//  3) Wait 10 ms.
//  4) Retrieve the contents again.
//
//The contents from 2 and 4 should be guaranteed to be
//identical.

//============================================================
//================ Representation of Reader ==================
//============================================================

//- running: Holds 1 if the reader is running. Holds 0 if the
//  reader has stopped, and its contents can be safely read.
//- thread_running: Holds 1 if the thread is still executing.
typedef struct {
  FILE* stream;
  pthread_t thread;
  pthread_mutex_t mutex;
  char* buffer;
  stz_long length;
  stz_long capacity;
  stz_int stop_requested;
  stz_int running;
  stz_int thread_running;
  stz_int free_on_finish;
} ThreadedReader;

//============================================================
//==================== Free Resources ========================
//============================================================
//Internal function used to free all memory/resources used
//by the reader.

void free_threaded_reader_resources (ThreadedReader* reader){
  pthread_mutex_destroy(&reader->mutex);
  free(reader->buffer);
  free(reader);  
}

//============================================================
//============= Add a Character to the Buffer ================
//============================================================

void ensure_buffer_capacity (ThreadedReader* reader, int desired_cap){
  if(reader->capacity < desired_cap){
    //Compute new capacity by doubling current capacity.
    int new_cap = reader->capacity;
    while(new_cap < desired_cap)
      new_cap *= 2;
    //Reallocate the memory.
    reader->capacity = new_cap;
    reader->buffer = realloc(reader->buffer, new_cap);
  }
}

void push_char (ThreadedReader* reader, char c){
  //Implement this operation within a locked section.
  pthread_mutex_lock(&reader->mutex);

  //Only add to the buffer if stop was not requested.
  if(!reader->stop_requested){
    //Ensure the buffer is large enough.
    ensure_buffer_capacity(reader, reader->length + 1);
    
    //Add to the buffer and increment the length.
    reader->buffer[reader->length] = c;
    reader->length += 1;
  }

  pthread_mutex_unlock(&reader->mutex);
}

//============================================================
//================== Reader Thread ===========================
//============================================================

void* reader_thread (void* p){
  //Assume that we passed in the ThreadedReader during pthread_create.
  ThreadedReader* reader = p;

  //Main reader loop: Continuously read characters from the stream,
  //as long as stop is not requested.
  while(!reader->stop_requested){
    //Read the next character. Will block.
    int c = fgetc(reader->stream);
    //If we reached the end of the stream, then
    //exit the loop.
    if(c == EOF){
      break;
    }
    //Otherwise, we read a successful character.
    else{
      //Add the character to our buffer.
      push_char(reader, (char)c);
    }
  }

  //The loop has finished, which means that we
  //reached the end of the stream.
  reader->running = 0;

  //Perform the following atomic operation:
  //  reader->thread_running = 0;
  //  int free_resources = reader->free_on_finish;
  pthread_mutex_lock(&reader->mutex);
  reader->thread_running = 0;
  int free_resources = reader->free_on_finish;
  pthread_mutex_unlock(&reader->mutex);

  //Clean up the thread resources if appropriate.
  if(free_resources)
    free_threaded_reader_resources(reader);

  //No specific return value.
  return NULL;
}

//============================================================
//================= Creation =================================
//============================================================

//Create a ThreadedReader to read from the specified stream.
ThreadedReader* make_threaded_reader (FILE* stream) {
  //Allocate the reader.
  ThreadedReader* reader = (ThreadedReader*)malloc(sizeof(ThreadedReader));
  reader->stream = stream;
  reader->capacity = 1024;
  reader->buffer = (char*)malloc(reader->capacity);
  reader->length = 0;
  reader->stop_requested = 0;
  reader->running = 1;
  reader->thread_running = 1;
  reader->free_on_finish = 0;

  //Start the thread, and return null if unsuccessful.
  int thread_ret = pthread_create(&reader->thread, NULL, reader_thread, reader);
  if(thread_ret != 0)
    goto failure_cleanup_reader;

  //Create the mutex.
  int mutex_ret = pthread_mutex_init(&reader->mutex, NULL);
  if(mutex_ret != 0)
    goto failure_cleanup_reader;

  //Return either the reader or null depending on success.
  if(thread_ret == 0) return reader;
  else goto failure_cleanup_reader;

  //Cleanup code
  failure_cleanup_reader:
  free(reader->buffer);
  free(reader);
  return NULL;
}

//============================================================
//================= Request Stop =============================
//============================================================

void stop_threaded_reader (ThreadedReader* reader){
  pthread_mutex_lock(&reader->mutex);
  reader->stop_requested = 1;
  reader->running = 0;
  pthread_mutex_unlock(&reader->mutex);
}

//============================================================
//============== Test whether Running ========================
//============================================================

//Returns true if the reader has *effectively* stopped running,
//and that the buffer is safe to be read from.
//Note that it does not mean that the thread has finished executing.
stz_int threaded_reader_running (ThreadedReader* reader){
  return reader->running;
}

//============================================================
//=============== Retrieve Buffer Contents ===================
//============================================================

//Retrieve a pointer to the contents of the buffer.
//Can only be used when the reader is not running.
char* threaded_reader_buffer (ThreadedReader* reader){
  return reader->buffer;
}

stz_long threaded_reader_buffer_length (ThreadedReader* reader){
  return reader->length;
}

//============================================================
//================== Cleanup =================================
//============================================================
//If the thread is still running, then we flag the thread
//to clean up the resources at the end.
//If the thread is not running, then we clean up the resources
//ourselves.

void delete_threaded_reader (ThreadedReader* reader){
  //Perform the following atomic operation:
  //  int running = reader->thread_running;
  //  if(running) reader->free_on_finish = 1;
  pthread_mutex_lock(&reader->mutex);
  int running = reader->thread_running;
  if(running) reader->free_on_finish = 1;
  pthread_mutex_unlock(&reader->mutex);

  //Clean up the resources if the thread is
  //not running.
  if(!running)
    free_threaded_reader_resources(reader);
}
