#include "glthread.h"
#include <stdlib.h>
#include <stdio.h>

void init_glthread(glthread_t *glthread) {
    if (glthread == NULL) {
        printf("Sorry init glthread failed");
        return;
    }

    glthread->left = NULL;
    glthread->right = NULL;
}

void glthread_add_next(glthread_t *curr_glthread, glthread_t *new_glthread){
    if (!curr_glthread->right) {
        curr_glthread->right = new_glthread;
        new_glthread->left = curr_glthread;
        return;
    }

    glthread_t *temp = curr_glthread->right;
    curr_glthread->right = new_glthread;
    new_glthread->left = curr_glthread;
    new_glthread->right = temp;
    temp->left = new_glthread;
}

void glthread_add_before(glthread_t *curr_glthread, glthread_t *new_glthread){
    if (!curr_glthread->left) {
        curr_glthread->left = new_glthread;
        new_glthread->right = curr_glthread;
        return;
    }

    glthread_t *temp = curr_glthread->left;
    curr_glthread->left = new_glthread;
    new_glthread->right = curr_glthread;
    new_glthread->left = temp;
    temp->right = new_glthread;
}

void glthread_add_last(glthread_t *base_glthread, glthread_t *new_glthread){
    glthread_t *glthreadptr = NULL, *prevglthreadptr = NULL;

    ITERATE_GLTHREAD_BEGIN(base_glthread, glthreadptr){
        prevglthreadptr = glthreadptr;
    } ITERATE_GLTHREAD_END(base_glthread, new_glthread);

    if (prevglthreadptr) {
        glthread_add_next(prevglthreadptr, new_glthread);
    } else {
        glthread_add_next(base_glthread, new_glthread);
    }
}

void remove_glthread(glthread_t *glthread){
    if (glthread == NULL) {
        return;
    }

    if (glthread->right != NULL) {
        glthread->right->left = glthread->left;
    }

    if (glthread->left != NULL) {
        glthread->left->right = glthread->right;
    }

    free(glthread);
}

void delete_glthread_list(glthread_t *base_glthread){
    glthread_t *glthreadptr = NULL;
               
    ITERATE_GLTHREAD_BEGIN(base_glthread, glthreadptr){
        remove_glthread(glthreadptr);
    } ITERATE_GLTHREAD_END(base_glthread, glthreadptr);
}

unsigned int get_glthread_list_count(glthread_t *base_glthread){
    
    unsigned int count = 0;
    glthread_t *glthreadptr = NULL;

    ITERATE_GLTHREAD_BEGIN(base_glthread, glthreadptr){
        count++;
    } ITERATE_GLTHREAD_END(base_glthread, glthreadptr);

    return count;
}

void glthread_priority_insert(glthread_t *base_glthread, 
                         glthread_t *glthread,
                         int (*comp_fn)(void *, void *),
                         int offset){


    glthread_t *curr = NULL,
               *prev = NULL;

    init_glthread(glthread);

    if(IS_GLTHREAD_LIST_EMPTY(base_glthread)){
        glthread_add_next(base_glthread, glthread);
        return;
    }

    ITERATE_GLTHREAD_BEGIN(base_glthread, curr){

        if(comp_fn(GLTHREAD_GET_USER_DATA_FROM_OFFSET(glthread, offset), 
                GLTHREAD_GET_USER_DATA_FROM_OFFSET(curr, offset)) == -1){
            glthread_add_before(curr, glthread);
            return;
        }
        
        prev = curr;
    }ITERATE_GLTHREAD_END(base_glthread, curr);

    glthread_add_next(prev, glthread);
}

#if 0
void *
gl_thread_search(glthread_t *base_glthread, 
                 void *(*thread_to_struct_fn)(glthread_t *), 
                 void *key, 
                 int (*comparison_fn)(void *, void *)){

    return NULL;
}

#endif