#ifndef __UVE_SE_TREE_UTILS_HH__
#define __UVE_SE_TREE_UTILS_HH__

#include "stdio.h"
#include "assert.h"

template <class Container>
struct node {
    node * next;
    node * sibling;
    node * prev;
    Container * content;
};

template <class Container>
class SEList
{
    public:
        using tnode = node<Container>;
        SEList(){head = NULL;}
        ~SEList(){destroy_list();}

        tnode * get_next(tnode *nd){
            return nd->next!=nullptr?
                get_next(nd->next) : nd;
        }
        tnode * get_head(){ return head;}
        tnode * get_end(){ return get_next(head);}
        void destroy_list(){
            destroy_list(head);
        };
        std::string to_string();

        void insert_dim(Container * content);
        void insert_mod(Container * content);

    private:
        void destroy_list(tnode * nd){
            if(nd!=NULL)
            {
                destroy_list(nd->next);
                delete (nd->sibling);
                delete nd;
            }
        }

        void insert(Container * content, tnode * nd, bool dim);
        std::string to_string(tnode * nd, std::string str);

        tnode *head;

};

template <class Container>
std::string SEList<Container>::to_string(){
    std::string str = "●\n";
    if(head != nullptr){
        str = to_string(head, str);
    }
    return str;
};

template <class Container>
std::string SEList<Container>::to_string(tnode * nd, std::string str){
    //Print node and siblings
    // D---M
    // |  /
    // | /
    // D
    std::string node_str = "";
    if(nd->sibling != nullptr){
        node_str += "D---M\n" "|  /\n" "| /\n";
    }
    else{
        node_str += csprintf("D  :%s\n",nd->content->to_string());
        node_str += "|\n" "|\n";

    }

    if(nd->next != nullptr){
        return to_string(nd->next, str + node_str);
    }
    else{
        return str + node_str;
    }
};


template <class Container>
void SEList<Container>::insert(Container * content, tnode * nd,
                                    bool dim){
    if(dim){
        if(nd->next == nullptr){
            tnode * new_nd = new tnode;
            new_nd->content = content;
            new_nd->next = nullptr;
            new_nd->sibling = nullptr;
            new_nd->prev = nd;
            nd->next = new_nd;
            if(nd->sibling != nullptr){
                nd->sibling->next = new_nd;
            }
        }
        else insert(content, nd->next, true);
    }
    else {
        if(nd->next == nullptr){
            assert(nd->sibling == nullptr);
            tnode * new_nd = new tnode;
            new_nd->content = content;
            new_nd->next = nullptr;
            new_nd->sibling = nullptr;
            new_nd->prev = nd;
            nd->sibling = new_nd;
        }
    }
}

template <class Container>
void SEList<Container>::insert_dim(Container * content){
    if(head != nullptr){
        insert(content, head, true);
    }
    else {
        head = new tnode;
        head->content = content;
        head->next = nullptr;
        head->sibling = nullptr;
        head->prev = nullptr;
    }
}

template <class Container>
void SEList<Container>::insert_mod(Container * content){
    assert(head != nullptr);
    insert(content, head, false);
}


#endif //__UVE_SE_TREE_UTILS_HH__