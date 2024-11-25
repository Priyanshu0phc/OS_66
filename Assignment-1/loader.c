#include "loader.h"

Elf32_Ehdr *ehdr;
Elf32_Phdr *phdr;
int fd;

/*
 * release memory and other cleanups
 */
void loader_cleanup() {
  if (ehdr) free(ehdr);
  if(phdr) free(phdr);
  close(fd);
//Freeing up space and closing file..
}

/*
 * Load and run the ELF executable file
 */
void load_and_run_elf(char** argv) {
  fd = open(argv[1], O_RDONLY); //Opening Elf file and assingning file discriptor to variable fd..
  if (fd==-1){ //fd have a non-negative integer value for succesful file open, and -1 for insuccesful
    perror("ERROR -file cant be opened \n"); //Error Printing incase file don't exist, or fails to be opened..
    exit(1);//Exit error_code=1
  }
  // 1. Load entire binary content into the memory from the ELF file.
  ehdr=(Elf32_Ehdr*) malloc(sizeof(Elf32_Ehdr)); //allocating space for Elf header to be stored and pointing it's starting with ehdr_p..
  read(fd,ehdr,sizeof(Elf32_Ehdr)); //reading first 52 bytes(size of 32 bit elf Header), from fd pointed Elf file..
  //First 52bytes in fd is nothing but elf header which we are storing into ehdr pointed structure.

  lseek(fd,ehdr->e_phoff, SEEK_SET); //Moving fd to starting of program header table, from seek_set(0,starting) to 0+phoff(starting of program headers)..
  phdr=(Elf32_Phdr*) malloc(sizeof(Elf32_Phdr)); // Allocating Space for program header to be stored, space being pointed by pointer phdr(global variable)..


  // 2. Iterate through the PHDR table and find the section of PT_LOAD 
  //    type that contains the address of the entrypoint method in fib.c
  for (int i = 0; i < ehdr->e_phnum; i++) { //iterating through, all progam headers. e_phnum(total no of program header in the elf file)
    read(fd,phdr, sizeof(Elf32_Phdr));// storing program header of index i in structure pointed by phdr..
    if (phdr->p_type == PT_LOAD && ehdr->e_entry >= phdr->p_vaddr && ehdr->e_entry < phdr->p_vaddr + phdr->p_memsz) {
//if program header stored inside is that segment that have entry point in it, and if it exist than obviously, it will be of type PT_LOAD..
// Let that segment reside into phdr that's why we have having break statement, to avoid any further updates..
        break;
    }
  }
/*
How this loop iterates over segments(Checks) Explained:
At starting of loop it points to starting, than after each iteration we move file pointer by size of a program header,
at the start of 1st iteration(v. start) we are pointing to the start of 1st segment.
at the end of 1st iteration/starting of 2nd iteration, we file pointer is pointing to the start of 2nd segment.
Simillarly, at the end of 2nd iteration/staring of 3rd one is we are pointing to start of 3rd segment and then reading it, from there.
And it goes on till we find segment having entry point...
*/
  // 3. Allocate memory of the size "p_memsz" using mmap function 
  //    and then copy the segment content
  void* virtual_m = mmap(NULL, phdr->p_memsz, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_ANONYMOUS | MAP_PRIVATE, 0, 0); //Given code for allocating virtual memory in process space, for loading choosen segment.

  lseek(fd, phdr->p_offset, SEEK_SET);// Now pointing to the start of that choosen segment.
  read(fd,virtual_m, phdr->p_memsz); //Reading that whole segment, and storing it to the virtual memory allocated..

  // 4. Navigate to the entrypoint address into the segment loaded in the memory in above step
  // 5. Typecast the address to that of function pointer matching "_start" method in fib.c.
  // 6. Call the "_start" method and print the value returned from the "_start"

  int offset = ehdr->e_entry - phdr->p_vaddr;// Calculating after how many bytes from staring of segment, entry point is, and that's called segment offset.
  void* entry = virtual_m + offset; //Pointing a pointer called entry to a point in virtual memory, which is offset ahead of the starting point, to virtual memory.
	// entry pointer points to entry point, where execution should begin in virtual memory.. 

  int (*_start)() = (int (*)()) entry; //Typecasting..
  int result = _start();
  printf("User _start return value = %d\n", result);
}

int main(int argc, char** argv) 
{
  if(argc != 2) {
    printf("Usage: %s <ELF Executable> \n",argv[0]);
    exit(1);
  }
  // 1. carry out necessary checks on the input ELF file
  // 2. passing it to the loader for carrying out the loading/execution
  load_and_run_elf(argv);
  // 3. invoke the cleanup routine inside the loader  
  loader_cleanup();
  return 0;
}
