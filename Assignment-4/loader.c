#include <elf.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
// Header files used in this code..

//function definations given here to avoid function precedence issues..
void print_data();
void execute_elf(char **argv);
Elf32_Phdr* find_segment(unsigned long addr);
void setup();
void loader_cleanup();
void segmentation_handler(int signal, siginfo_t *info, void *context);
//Some constants defined..
#define P_SIZE 4096 //Size of a page in Linux OS
#define MAX_DATA 100
//Some Data Structures to hold program headers, and Elf header..
Elf32_Phdr *phdr;
Elf32_Ehdr *ehdr;
int fd;
unsigned long fault_addresses[MAX_DATA];
int pointer_f=0;
// Page fault statistics
int numPageFaults;  // Total page faults
int numPageAllocations;  // Total page allocations
long long internalFragmentation;  // Internal fragmentation in bytes..

// Function to get Segment which have the fault address..
Elf32_Phdr* find_segment(unsigned long addr){
    lseek(fd,ehdr->e_phoff,SEEK_SET);
    int len=ehdr->e_phnum;
    for(int i=0; i<len; i++){
        if(read(fd, phdr, sizeof(Elf32_Phdr))!=sizeof(Elf32_Phdr)) {
            free(phdr);
            return NULL;
        }
        if (phdr->p_type==PT_LOAD && addr < phdr->p_vaddr +phdr->p_memsz && addr >= phdr->p_vaddr) return phdr;
        //Condition same as the one used in first assingment..
    }
    return NULL;
}

// Signal handler for segmentation faults (page faults)
void segmentation_handler(int signal, siginfo_t *info, void *context) {
    numPageFaults++;
    unsigned long fault_addr = (unsigned long)info->si_addr; 
    fault_addresses[pointer_f]=fault_addr;
    pointer_f++;
    // Find the segment containing the fault address
    Elf32_Phdr *segment = find_segment(fault_addr);
    if(segment==NULL){
        fprintf(stderr, "No Such Segment Requested %lx\n", fault_addr);
        exit(1);
    }    
    unsigned long seg_off=(fault_addr/P_SIZE)*P_SIZE-segment->p_vaddr; //Offset inside segment calculated..
    unsigned long aligned_file_offset = segment->p_offset+seg_off;
    unsigned long page_start_address=(fault_addr/P_SIZE)*P_SIZE;
    // Map the memory page
    void *address_mapped=mmap( (void *)page_start_address,4096, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS,-1,0);
    if(address_mapped==MAP_FAILED){
        perror("Page Allocation Failed !");
        exit(1);
    }

    // Copy the segment data into the mapped memory from the file
    if (pread(fd, address_mapped, P_SIZE, aligned_file_offset)<0) {
        perror("Copy Operation on segment failed !");
        exit(1);
    }
    numPageAllocations++;
    internalFragmentation+=P_SIZE-(segment->p_memsz-seg_off<P_SIZE ? segment->p_memsz - seg_off:P_SIZE);
}
// Function to load and execute the ELF file
void execute_elf(char **exe) {
    fd = open(exe[1], O_RDONLY); //onening the elf file in read mode..
    if(fd==-1){ //fd return positive value after successful operation else -1, error handling
        perror("Failed to open ELF file\n");
        exit(1); //Terminating..
    }
    if (read(fd, ehdr, sizeof(Elf32_Ehdr))!=sizeof(Elf32_Ehdr)){ //It returns the size of data readed, if it's not same as the size then, it means 
    //wrong data readed, so exit or terminate program..
        perror("Elf Header Can't be readed..\n");
        close(fd);
        exit(1);
    }
    // Call the entry point of the ELF executable
    int (*_start)() = (int (*)())(ehdr->e_entry); //Type Casting given by Sir in very first assingment..
    int res = _start(); //Launched ELF execution..
    print_data(res); //Printer Invoked..
    close(fd); //file descriptor close, to avoid leakage of resourse..
}
// Function to clean up resources
void loader_cleanup(){
    if (phdr!= NULL) free(phdr); //Freeing up space alloacted to phdr at the begging in main() func..
    if (ehdr !=NULL) free(ehdr); //simillarly
}

void setup(){
    struct sigaction sa;
    memset(&sa, 0, sizeof(struct sigaction));
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction=segmentation_handler;
    if(sigaction(SIGSEGV, &sa, NULL)<0){ //this returns -1 as setup fails for a signal handler..
        perror("Failed to set up signal handler"); 
        exit(1); // Terminate as not going ahead..
    }
}
 
void print_data(int res){
    //Data Recorded by Our Smart Loader..
    printf("_start address Value = %d\n", res);
    printf("Total Page Faults = %d\n", numPageFaults);
    printf("Total Page Allocated %d\n", numPageAllocations);
    printf("Internally Space Fragmented: %lld Bytes \n", internalFragmentation);
    printf("Fault Occured at: \n");
    for(int i=0; i<numPageFaults; i++){
        printf("%d",i+1);
        printf(" %lu\n",fault_addresses[i]);
    }
}


int main(int argc, char **argv) {
    if(argc!=2){
        printf("Wrong Usage, enter exe name after ./loader\n");
        exit(1);
    }
    numPageAllocations=0; //THese are Global Variables..
    internalFragmentation=0;
    numPageFaults=0;
    //Initializing Variables to store correct values..

    // Allocating space to elf_header and program_headers..
    phdr=(Elf32_Phdr*)malloc(sizeof(Elf32_Phdr));
    if (phdr==NULL) { //Error Checking..., if found return 
        perror("Malloc Operation failed for Program Header..\n");
        exit(1); //Teminate as we can't proceed without phdr..
    }
    ehdr=(Elf32_Ehdr*)malloc(sizeof(Elf32_Ehdr));
    if(ehdr==NULL){ //Error Checking..., if found return 
        perror("Malloc Operation failed for ELF Header..\n");
        exit(1); //Teminate as we can't proceed without ehdr..
    }
    setup(); //Signal Handler to detect Segmentation Fault Signals, which will launch page fault...
    // Load and run the ELF executable
    execute_elf(argv); //Launching given elf file..
    // Clean up resources after execution
    loader_cleanup(); //Cleaning Up the space allocated to elf header and phdr, before terminating
    return 0; //Just a simple return stm. 
}
