# AkoC
A C implementation of the Ako config language.

For info about Ako see [Ako's README](https://github.com/Tuyuji/Ako/blob/main/README.md)

## Warning: Incomplete.
While a majority of this library works it's not properly tested, if you do intend to use this library and encounter 
issues please open an issue on the GitHub repository.

It's often tested with Valgrind so as long as you properly call free on the data you own you should be fine.

## Quick start
```c++
#include <ako/ako.h>

int main()
{
    const char* src = "window.size 800x600";
    ako_elem_t* root = ako_parse(src);
    if(ako_elem_is_error(root))
    {
        printf("Error: %s\n", ako_elem_get_string(root));
        ako_elem_destroy(root);
        return 1;
    }
    
    ako_elem_t* win = ako_elem_table_get(root, "window");
    ako_elem_t* size = ako_elem_table_get(win, "size");
    
    ako_int w = ako_elem_get_int(ako_elem_array_get(size, 0));
    ako_int h = ako_elem_get_int(ako_elem_array_get(size, 1));
    
    printf("Window size: %d x %d\n", w, h);
    
    ako_elem_table_add(win, "title", ako_elem_create_string("Hello, world"));
    
    const char* config = ako_serialize(root, NULL, ASF_FORMAT);
    printf("Serialized config:\n%s\n", config);
    ako_free_string(config);
    
    //Frees the root element and all its children
    ako_elem_destroy(root);
}
```

When you add to an element to a table or array you transfer ownership of that element over.

Please look at the tests for more examples of how to use the library.

### Custom allocation
You can override the default allocation functions used by Ako by modifying the `ako_alloc_t` struct from `ako_alloc_get()`
```c++
void setup_alloc()
{
    ako_alloc_t* alloc = ako_alloc_get();
    alloc->malloc = my_malloc;
    alloc->free = my_free;
    alloc->realloc = my_realloc;
    alloc->userdata = my_userdata;
}
```
