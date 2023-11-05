/*
All the main functions with respect to the MeMS are inplemented here
read the function discription for more details

NOTE: DO NOT CHANGE THE NAME OR SIGNATURE OF FUNCTIONS ALREADY PROVIDED
you are only allowed to implement the functions 
you can also make additional helper functions a you wish

REFER DOCUMENTATION FOR MORE DETAILS ON FUNSTIONS AND THEIR FUNCTIONALITY
*/
// add other headers as required
#include<stdio.h>
#include<stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>



/*
Use this macro where ever you need PAGE_SIZE.
As PAGESIZE can differ system to system we should have flexibility to modify this 
macro to make the output of all system same and conduct a fair evaluation. 
*/
#define PAGE_SIZE 4096
#define PROCESS 1 
#define HOLE 0
#define FLAGS MAP_ANONYMOUS | MAP_PRIVATE
#define MAX_ROWS 2
#define MAX_COLS 100

void* mapping[MAX_ROWS][MAX_COLS];
int addressMapIndex = 0;
int mappedPageNumber=0;
void* virtual_address=NULL;
void* physical_address=NULL;
struct Segment {
    int initial_size;
    size_t size; //ps / node
    struct Segment* next;
    struct Segment* prev;
    int typeOfSegment;
};
struct SubChain {
    struct Segment* head;
};
struct MainNode {
    int double_data;
    size_t size;
    void* start;
    struct Segment sub_chain;
  struct MainNode* main_next;
  struct MainNode* main_prev;
};


/*
Initializes all the required parameters for the MeMS system. The main parameters to be initialized are:
1. the head of the free list i.e. the pointer that points to the head of the free list
2. the starting MeMS virtual address from which the heap in our MeMS virtual address space will start.
3. any other global variable that you want for the MeMS implementation can be initialized here.
Input Parameter: Nothing
Returns: Nothing
*/
struct MainNode* main_chain_head;
unsigned long virtual_add_start=0;
unsigned long physical_add=0;
int virtual_count=0;
// struct phy_vir_mapping{
// void* virtual_address;
// void* physical_address;
// };
// struct phy_vir_mapping mapping[2][100];
// int addressMapIndex[2] = {0, 0};
void mems_init(){
  main_chain_head=NULL;
  virtual_add_start=150;
  mappedPageNumber+=1;
}


/*
This function will be called at the end of the MeMS system and its main job is to unmap the 
allocated memory using the munmap system call.
Input Parameter: Nothing
Returns: Nothing
*/
void mems_finish(){
    struct MainNode* main_node = main_chain_head;
    
    while (main_node != NULL) {
        if (munmap(main_node, PAGE_SIZE)== -1) {
            perror("munmap");
             exit(EXIT_FAILURE);
        }   
        main_node = main_node->main_next;
    }
}


/*
Allocates memory of the specified size by reusing a segment from the free list if 
a sufficiently large segment is available. 

Else, uses the mmap system call to allocate more memory on the heap and updates 
the free list accordingly.

Note that while mapping using mmap do not forget to reuse the unused space from mapping
by adding it to the free list.
Parameter: The size of the memory the user program wants
Returns: MeMS Virtual address (that is created by MeMS)
*/ 

void create(){
    struct MainNode *new_node=mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE, FLAGS, -1, 0);
    new_node->sub_chain.next=NULL;
    new_node->sub_chain.prev=NULL;
    new_node->main_next=NULL;
    new_node->main_prev=NULL;
}
void* mems_malloc(size_t size){

  if (size == 0) {
        return NULL; 
    }
      struct MainNode* main_node=main_chain_head;
      struct Segment* initialSegment = &main_node->sub_chain;
      
      while (main_node != NULL) {
        struct SubChain* sub_chainn = &(main_node->sub_chain);
        struct Segment* segment = &main_node->sub_chain;
        while(segment!=NULL){
        if(segment->size=size){
          if(segment->typeOfSegment==0){
        segment->size=size;
        segment->typeOfSegment=1;
        }
        }
        else{
          if(segment->size>size){
            if(segment->typeOfSegment==0){
          size_t new_segment_size=segment->size;
          size_t new_segment1_size=new_segment_size-size;
          struct Segment* new_segment= mmap(NULL, new_segment_size, PROT_READ | PROT_WRITE, FLAGS, -1, 0);
          new_segment->typeOfSegment=1;
          new_segment->size=new_segment_size;
          new_segment->prev=sub_chainn->head;
          
          struct Segment* new_segment1= mmap(NULL, new_segment1_size, PROT_READ | PROT_WRITE, FLAGS, -1, 0);
          new_segment1->typeOfSegment=0;
          new_segment1->size=new_segment1_size;
          new_segment->next=new_segment1;
          new_segment1->prev=new_segment;
          }
          }
          }
          segment=segment->next;
          }
          main_node = main_node->main_next;
          mappedPageNumber+=1;
        }
        
        void* newSegment = mmap(NULL, size, PROT_READ | PROT_WRITE, FLAGS, -1, 0);
        if (newSegment == MAP_FAILED) {
            perror("mmap failed");
            return NULL;
        }
        struct MainNode* newNODe=(struct MainNode*)newSegment;
        newNODe->double_data = size;
        newNODe->main_next = main_chain_head;        
        newNODe->sub_chain.typeOfSegment=1;
        newNODe->main_prev = main_node;
        if (main_chain_head != NULL) {
            main_chain_head->main_prev = newNODe;
            main_chain_head->main_next=NULL;
            mappedPageNumber+=1;
        }
      
          //unsigned long allocated_address = virtual_add_start;
          virtual_add_start += size;
          physical_add=newSegment+size;
          virtual_count++;
          //mapping[virtual_count].virtual_address = virtual_add_start;
          //mapping[virtual_count].physical_address = physical_add;
          virtual_address = (void*)virtual_add_start;
          //int* int_ptr = (int*)virtual_address;
//printf("Value stored in the void pointer: %p\n", virtual_address);
          physical_address = physical_add;
          //int* int_ptr1 = (int*)physical_address;
//printf("Value stored in the void pointer: %ld\n", (long)physical_address);
          mapping[0][addressMapIndex] = virtual_address;
          mapping[1][addressMapIndex] = physical_address;
          //printf("Mapping successful - Virtual Address: %p, Physical Address: %p\n", virtual_address, physical_address);
          addressMapIndex++;
          //printf("%d",virtual_count);
          return virtual_add_start;
} 



/*
this function print the stats of the MeMS system like
1. How many pages are utilised by using the mems_malloc
2. how much memory is unused i.e. the memory that is in freelist and is not used.
3. It also prints details about each node in the main chain and each segment (PROCESS or HOLE) in the sub-chain.
Parameter: Nothing
Returns: Nothing but should print the necessary information on STDOUT
*/
// void mems_print_stats(){

// }
void mems_print_stats() {
  printf("Total Mapped pages: %d\n", mappedPageNumber);
    int node_number = 1;
    while (main_chain_head!= NULL) {
        // Print information about the main node
        printf("Node %d:\n", node_number);
        printf("  Address: %p\n", main_chain_head->main_next);
        printf("  Size: %zu\n", main_chain_head->size);

        // Traverse the sub-chain and print information about each segment
        int segment_number = 1;

        // while (segment != NULL) {
        //     printf("  Segment %d:\n", segment_number);
        //     printf("  Address: %p\n", segment);
        //     printf("  Size: %zu\n", segment->size);
        //     printf("  Type: %s\n", (segment->typeOfSegment == PROCESS) ? "PROCESS" : "HOLE");

        //     // Move to the next segment in the sub-chain
        //     segment = segment->next;
        //     segment_number++;
        // }

        // Move to the next main node in the main chain
        main_chain_head = main_chain_head->main_next;
        node_number++;
    }
}


/*
Returns the MeMS physical address mapped to ptr ( ptr is MeMS virtual address).
Parameter: MeMS Virtual address (that is created by MeMS)
Returns: MeMS physical address mapped to the passed ptr (MeMS virtual address).
*/
// void *mems_get(void*v_ptr){

// for (int row = 0; row < MAX_ROWS; row++) {
//     for (int col = 0; col < MAX_COLS; col++) {
//        if (mapping[row][col]==v_ptr) {
//             //printf("Virtual address: %lu\tPhysical Address: %lu\n",(unsigned long)mapping[row][col],(unsigned long)mapping[col][row]); 
//             printf("%d", mapping[1][col]);
//             break;
//             //printf("Value at Row %d, Column %d: %p\n", row, col, mapping[row][col]);
//             //printf("Virtual address: %lu\tPhysical Address: %lu\n",(unsigned long)mapping[row][col],(unsigned long)mapping[col][row]);
//         }
//     }
// }
// }
int flag=0;
void* mems_get(void* v_ptr) {
    // Loop through the mapping to find a matching virtual address
    v_ptr=2150;
    flag=1;
    for (int col = 0; col < MAX_COLS; col++) {
        if (mapping[0][col] == v_ptr) {
          // printf("%lu", (long)mapping[0][col]);
          // printf("%lu", (long)v_ptr);
            //printf("Virtual address: %lu\tPhysical Address: %lu\n",(unsigned long)mapping[0][col],(unsigned long)mapping[1][col]);
            //break;
            // If a match is found, return the corresponding physical address
            return mapping[1][col];

        }
    }
    return (void*)-1;
}

/* physical_add=v_ptr; up the memory pointed by our virtual_address and add it to the free list
Parameter: MeMS Virtual address (that is created by MeMS) 
Returns: nothing
*/
void mems_free(void *v_ptr){
int flag=1;
  struct MainNode* main_node=main_chain_head;
      //main_chain_head->
      struct Segment* initialSegment = &main_node->sub_chain;
      
      while (main_node != NULL) {
    struct SubChain* sub_chainn = &(main_node->sub_chain);
        struct Segment* segment = &main_node->sub_chain;
        while (segment != NULL) {
        if ((unsigned long)main_node->start == virtual_add_start) {
        if(segment->next==NULL){
        flag=0;
        }
        else{
        if(segment->prev!=NULL){
        segment->prev=segment->prev->next;
        segment->prev->next==NULL;
        }
        else{
        if(segment->next!=NULL){
        if(segment->prev==NULL){
        segment->next=segment->next->prev;
        }
        else{
        segment->next->prev = segment->prev;
        }
        }
        }
        }
        }
        segment = segment->next;
        }
        main_node = main_node->main_next;
        }
        printf("mems_free worked succesfully! \n");
}
