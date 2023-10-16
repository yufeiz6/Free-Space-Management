#include <iostream>
#include <string>
using namespace std;

struct Freenode {
    int addr;
    int size;
    Freenode* prev;
    Freenode* next;
};

struct Freelist {
    Freenode* head;
    Freenode* last_searched;  // For Next Fit
    int length;
};

struct Header {
    int hdptr;
    int allocptr;
    int size;
};

struct Allocator {
    int base;
    int size;
    Freelist free_list;
};

Freelist init_freelist(int base, int size) {
    Freelist list;
    Freenode* node = new Freenode{base, size, NULL, NULL};
    list.head = node;
    list.last_searched = NULL;
    list.length = 1;
    return list;
}

Header init_header(int hdptr, int size) {
    Header header;
    header.hdptr = hdptr;
    header.allocptr = hdptr + sizeof(Header);
    header.size = size;
    return header;
}

Allocator init_allocator(int base, int size) {
    Allocator allocator;
    allocator.base = base;
    allocator.size = size;
    allocator.free_list = init_freelist(base, size);
    return allocator;
}

Freenode* search(Freelist &list, int size_requested) {
    if (!list.head) {
        return NULL;  // Return NULL if the list is empty.
    }

    if (list.last_searched == NULL) {
        list.last_searched = list.head;
    }

    Freenode* current = list.last_searched;
    Freenode* start = list.last_searched;  // Initialize start to last_searched

    do {
        // If the current node has enough size, return it.
        if (current->size >= size_requested) {
            list.last_searched = current;
            return current;
        }

        // Move to the next node.
        current = current->next;

        // If we've reached the end of the list, wrap around to the start.
        if (!current) {
            current = list.head;
        }

    } while (current != start);  // If we've checked all nodes and haven't found a fit, break.

    return NULL;  
}

void insertNode(Freelist &list, int addr, int size) {
    // Insert based on address. (insert then merge)
    Freenode* newNode = new Freenode{addr, size, NULL, NULL};
    
    if (!list.head) {
        list.head = newNode;
        return;
    }

    Freenode* current = list.head;
    Freenode* previous = NULL;

    while (current && current->addr < addr) {
        previous = current;
        current = current->next;
    }

    // Insert the new node between previous and current.
    newNode->next = current;
    newNode->prev = previous;
    list.length++;

    if (previous) {
        previous->next = newNode;
    } else {
        // If there's no previous, then the new node becomes the head.
        list.head = newNode;
    }

    if (current) {
        current->prev = newNode;
    }

    // Merge with previous node if adjacent.
    if (newNode->prev && newNode->prev->addr + newNode->prev->size == newNode->addr) {
        newNode->prev->size += newNode->size;
        newNode->prev->next = newNode->next;
        if (newNode->next) {
            newNode->next->prev = newNode->prev;
        }
        Freenode* toDelete = newNode;
        newNode = newNode->prev;
        delete toDelete;
        list.length--;
    }

    // Merge with next node if adjacent.
    if (newNode->next && newNode->addr + newNode->size == newNode->next->addr) {
        newNode->size += newNode->next->size;
        Freenode* toDelete = newNode->next;
        newNode->next = newNode->next->next;
        if (newNode->next) {
            newNode->next->prev = newNode;
        }
        delete toDelete;
        list.length--;
    }

    // If we've inserted before the last_searched node, update last_searched.
    if (list.last_searched && newNode->addr < list.last_searched->addr) {
        list.last_searched = newNode;
    }
}

void removeNode(Freelist &list, Freenode* node) {
    if (node->prev) {
        node->prev->next = node->next;
    } else {
        list.head = node->next;
    }

    if (node->next) {
        node->next->prev = node->prev;
    }

    if (list.last_searched == node) {
        list.last_searched = NULL; 
    }

    delete node;
    list.length--;
}

int split(Freelist &list, Freenode* node, int size_requested) {
    if (node->size == size_requested) {
        int addr = node->addr;
        removeNode(list, node);
        return addr;
    } else {
        int hdptr = node->addr;
        node->addr += size_requested;
        node->size -= size_requested;
        return hdptr;
    }
}

Header* alloc(Allocator &allocator, int size) {
    int size_with_header = size + sizeof(Header);
    Freenode* node = search(allocator.free_list, size_with_header);
    if (node) {
        int hdptr = split(allocator.free_list, node, size_with_header);
        Header* header = new Header{hdptr, size};
        return header;
    } else {
        return NULL;
    }
}

void free(Allocator &allocator, Header* header) {
    int addr = header->hdptr;
    int size = header->size + sizeof(Header);
    insertNode(allocator.free_list, addr, size);
    delete header;
}

int main() {
    // Initialize the allocator with a base of 0 and size of 1000
    Allocator allocator = init_allocator(500, 1000);

    // Allocate 200 bytes of memory
    Header* header1 = alloc(allocator, 200);
    cout << "Allocated memory at: " << header1->allocptr << " with size: " << header1->size << endl;

    // Allocate another 200 bytes of memory
    Header* header2 = alloc(allocator, 200);
    cout << "Allocated memory at: " << header2->allocptr << " with size: " << header2->size << endl;

    // Free the first allocation
    free(allocator, header1);
    cout << "Freed memory at: " << header1->hdptr << endl;

    // Allocate 100 bytes of memory
    Header* header3 = alloc(allocator, 100);
    cout << "Allocated memory at: " << header3->allocptr << " with size: " << header3->size << endl;

    // Displaying the free list
    Freenode* node = allocator.free_list.head;
    while (node) {
        cout << "Free block at: " << node->addr << " with size: " << node->size << endl;
        node = node->next;
    }

    // Clean up the remaining headers
    free(allocator, header2);
    free(allocator, header3);

    return 0;
}





