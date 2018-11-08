//
// Created by Colin MacKenzie on 5/18/17.
//

#include "binbag.h"

#include <stdlib.h>
#include <assert.h>
#include <stdio.h>


#if defined(HAS_STRINGS_H)
#include <strings.h>
#endif

#if defined(HAS_STRING_H)
#include <string.h>
#endif

// std includes cannot have inline defined as a macro
#ifdef inline
#undef inline
#endif
#include <algorithm>

#define binbag_elements_end

// the number of protective fenceposts between the string data and the string array
#define FENCEPOSTS  4

const uint32_t fencepost = 0xdefec8ed;

/* Fencepost routines
 * Enforce memory bounds between string data and string pointer array against accidental writes by writing a fencepost
 * value between the two buffers and checking the values dont change before and after string table inserts.
 */
#if defined(FENCEPOSTS) && FENCEPOSTS>0
void check_fencepost(binbag* bb)
{
    uint32_t* fpmem = (uint32_t*)binbag_begin_iterator(bb) - FENCEPOSTS;
    for(int i=0; i<FENCEPOSTS; i++)
        if(fpmem[i] != fencepost) {
            size_t slen = 0;
            printf("binbag: fencepost was corrupted at position %d, expected %08x, got %08x\n", i, fencepost, fpmem[i]);
            binbag_debug_print(bb);
            assert(fpmem[i] == fencepost);
        }
}

void write_fencepost(binbag* bb)
{
    uint32_t* fpmem = (uint32_t*)binbag_begin_iterator(bb) - FENCEPOSTS;
    for(int i=0; i<FENCEPOSTS; i++)
        fpmem[i] = fencepost;
}

void binbag_debug_print(binbag* bb)
{
    uint32_t* fpmem = (uint32_t*)binbag_begin_iterator(bb) - FENCEPOSTS;
    size_t N;
    char s[1000];
    size_t slen = 0;
    long long offset;
    printf("memory contents of binbag: %lu elements, %lub of %lub capacity used   (freespace %lu)\nelements:\n",
           N=binbag_count(bb), binbag_byte_length(bb), binbag_capacity(bb), binbag_free_space(bb));
    for(size_t j=0; j<N; j++) {
        const char* el = binbag_get(bb, j);

        if(el==NULL) {
            strcpy(s, "NULL!");
            offset = 0;
            slen = 0;
        } else if(el < bb->begin || el > bb->tail) {
            sprintf(s, "out-of-bounds!   addr=0x%08llx", (unsigned long long) el);
            offset = 0;
            slen = 0;
        } else {
            slen = strlen(el);
            offset = el - bb->begin;
            if(slen<80)
                strcpy(s, el);
            else {
                strncpy(s, el, 40);
                s[40]=s[41]=s[42]='.';
                strcpy(&s[43], el + slen - 37);
            }
        }
        printf("   %4lu: @%08llu %4lub %s\n", j, offset, slen, s);
    }

    // print fenceposts
#if defined(FENCEPOSTS) && FENCEPOSTS >0
    bool ok = true;
    printf("fenceposts:");
    for(int j=0; j<FENCEPOSTS; j++) {
        if(fpmem[j] != fencepost)
            ok = false;
        printf(" 0x%08x", fpmem[j]);
    }
    printf("   %s\n", ok ? "OK":"CORRUPT");
#endif
}
#else
#define FENCEPOSTS  0
#define check_fencepost(bb)
#define write_fencepost(bb)
void binbag_debug_print(binbag* bb) { printf("binbag: fencepost checking disabled.\n"); }
#endif

binbag* binbag_create(size_t capacity_bytes, double growth_rate)
{
    if(capacity_bytes<32)
        capacity_bytes = 32;
    binbag* bb = (binbag*)calloc(1, sizeof(binbag));
    capacity_bytes += FENCEPOSTS*sizeof(fencepost); // fencepost DMZ
    bb->begin = bb->tail = (char*)malloc(capacity_bytes);
    bb->end = bb->begin + capacity_bytes;
    bb->elements = (const char**)bb->end;
    bb->growth_rate = growth_rate;
    bb->growths = 0;
    write_fencepost(bb);
    check_fencepost(bb);
    return bb;
}

binbag* binbag_split_string(int seperator, unsigned long flags, const char* str)
{
    int slen=0, scnt = *str ? 1 : 0;
    const char *p = str, *s = str;
    while(*p) {
        slen++;
        if (*p==seperator) {
            scnt++;
            s = p;
        }
        p++;
    }
    binbag* bb = binbag_create(slen + scnt*(sizeof(char*)+1), 1.5);
    if(scnt>0) {
        p = s = str;
        while (*p) {
            if (*p == seperator) {
                if(p>s || (flags & SF_IGNORE_EMPTY)==0)
                    binbag_insertn(bb, s, p-s);
                s = p+1;
            }
            p++;
        }
        if(p>s || (flags & SF_IGNORE_EMPTY)==0)
            binbag_insertn(bb, s, p-s);
    }
    return bb;
}

//binbag* binbag_create_from_array(const char** arr, size_t count, double fill_space, double growth_rate);

void binbag_free(binbag* bb)
{
    // just a sanity check
    assert(bb->end > bb->begin);
    free(bb->begin);
    free(bb);
}


size_t binbag_byte_length(binbag* bb)
{
    return bb->tail - bb->begin;
}

size_t binbag_count(binbag* bb)
{
    return binbag_end_iterator(bb) - binbag_begin_iterator(bb);
}

size_t binbag_capacity(binbag* bb)
{
    return (char*)bb->elements - bb->begin - FENCEPOSTS*sizeof(fencepost);
}

size_t binbag_resize(binbag* bb, size_t capacity)
{
    size_t _tail = bb->tail - bb->begin;
    size_t _end = bb->end - bb->begin;
    size_t _elements_offset = (char*)bb->elements - bb->begin;
    size_t _count = binbag_count(bb);
    size_t _existing_capacity = binbag_capacity(bb);
    size_t min_capacity = _count*sizeof(const char*) + _tail;
    if(capacity < min_capacity)
        capacity = min_capacity;

    //printf("growing from %d to %lu with %d elements\n", _existing_capacity, capacity, _count);

    assert(_count >=0);
    assert(_end >= _elements_offset);
    assert(_elements_offset >= _tail);
    check_fencepost(bb);

    // increase capacity to fit the array
    capacity += + _count*sizeof(char*);

    char* _begin = bb->begin;
    bb->begin = (char*)realloc(bb->begin, capacity);
    bb->end = bb->begin + capacity;
    bb->tail = bb->begin + _tail;
    char* _pelements = (char*)binbag_end_iterator(bb) - sizeof(char*)*_count;
    bb->elements = binbag_end_iterator(bb) - _count;
    assert(_pelements == (char*)bb->elements);

    // move the elements to the end of the buffer again
    // must use memmove() since our memory may overlap
    if(_count>0) {
        if(_begin == bb->begin)
            // no change in memory location during relocation so we can simply move our array of elements
            memmove( (char*)bb->elements, bb->begin + _elements_offset, _count * sizeof(char *));
        else {
            // memory moved, so we need to relocate the strings in our array to point to the new memory
            //printf("   memory was moved\n");
            const char** pfrom = (const char**)(bb->begin + _elements_offset);
            const char** pto = bb->elements;
            while((char*)pto < bb->end) {
                *pto++ = bb->begin + (*pfrom++ - _begin);
            }
        }
    }

    assert(bb->end >= (char*)bb->elements);
    assert((char*)bb->elements >= bb->tail);
    assert(bb->tail >= bb->begin);
    write_fencepost(bb);    // update new fencepost DMZ
    bb->growths++;
    return capacity;
}

size_t binbag_free_space(binbag* bb)
{
    size_t free_space = (const char*)bb->elements - FENCEPOSTS* sizeof(fencepost) - bb->tail;
    return (free_space>0) ? free_space : 0;
}

size_t binbag_insert_distinct(binbag* bb, const char* str)
{
    const char* _el;
    for(size_t i=0, c=binbag_count(bb); i<c; i++)
        if(strcmp(_el=binbag_get(bb, i), str) ==0)
            return i;
    return binbag_insert(bb, str);
}

size_t binbag_insert(binbag* bb, const char* str)
{
    return binbag_insertn(bb, str, -1);
}

size_t binbag_insertn(binbag *bb, const char *str, int length)
{
    // trying to do this without needing a strlen() and a strcpy() operation
    bool all = false;
    int fs = (int)binbag_free_space(bb); // save 4 bytes for added array element and the null character
    check_fencepost(bb);
    const char *p = bb->tail;   // we'll return this as the string we just inserted
    char *_p = NULL;
    if(fs>0 && length<fs) {
        _p = stpncpy(bb->tail, str, (size_t)((length<0) ? fs : std::min(fs, length)));  // must use signed inner type
        size_t copied = _p - bb->tail;
        if(length>=0 && copied==length) {
            *_p = 0;
            all = true;
        } else
            all = (str[copied] == 0); // we should see the null marker terminated the string
        check_fencepost(bb);
    }

    if(!all) {
        // since we didnt get everything, we should do a strlen() to determine exact size and make sure we grow enough
        size_t slen = strlen(str);

        // grow the buffer
        size_t existing_capacity = binbag_capacity(bb);
        size_t new_capacity = (size_t)(existing_capacity * std::min(10.0, std::max(1.1, bb->growth_rate))) + slen;
        binbag_resize(bb, new_capacity);
        return binbag_insert(bb, str);
    }

    // accept the new string, add string pointer to elements, advance the tail insertion pointer
    size_t idx = binbag_count(bb);      // our new string index
    *--bb->elements = bb->tail;
    bb->tail = _p+1;
    write_fencepost(bb);
    return idx;
}

//const unsigned char* binbag_binary_insert(const unsigned char* str, size_t len);

const char* binbag_get(binbag* bb, size_t idx)
{
    const char** e = (const char**)(bb->end - sizeof(char*)) - idx;
    return (e < binbag_end_iterator(bb))
        ? *e
        : NULL; // out of bounds
}

long binbag_find(binbag *bb, const char* match, int (*compar)(const char*,const char*))
{
    for(long j=0, N=binbag_count(bb); j<N; j++) {
        const char *el = binbag_get(bb, j);
        if(compar(match, el) ==0) {
            return j;
        }
    }
    return -1;
}

long binbag_find_case(binbag *bb, const char* match)
{
    return binbag_find(bb, match, strcmp);
}

long binbag_find_nocase(binbag *bb, const char* match)
{
    return binbag_find(bb, match, strcasecmp);
}

void binbag_inplace_reverse(binbag *bb)
{
    // reverse string table in place
    char **begin = (char**)binbag_begin_iterator(bb),
         **end = (char**)binbag_end_iterator(bb)-1; // actually 'last'
    while(begin < end) {
        // swap entries
        char* w = *begin;
        *begin++ = *end;
        *end-- = w;
    }
}

int binbag_element_sort_desc (const void * _lhs, const void * _rhs)
{
    // we get pointers to the elements, so since our elements are pointers then we must
    // dereference twice to get the resolver structure
    const char* lhs = *(const char**)_lhs;
    const char* rhs = *(const char**)_rhs;
    return strcmp(lhs, rhs);
}

int binbag_element_sort_asc (const void * _lhs, const void * _rhs)
{
    // we get pointers to the elements, so since our elements are pointers then we must
    // dereference twice to get the resolver structure
    const char* lhs = *(const char**)_lhs;
    const char* rhs = *(const char**)_rhs;
    return strcmp(rhs, lhs);    // swapped order
}

binbag* binbag_sort(binbag *bb, int (*compar)(const void*,const void*))
{
    check_fencepost(bb);
    size_t N = binbag_count(bb);

    // create a new binbag with just enough capacity to hold a copy of bb (but sorted)
    size_t capacity = binbag_byte_length(bb) + N*sizeof(char*);
    binbag* sorted = binbag_create(capacity, bb->growth_rate);

    // copy the string table index (this index will still reference strings from old table)
    sorted->elements = binbag_end_iterator(sorted) - N;
    assert((void*)sorted->elements > sorted->begin && (void*)sorted->elements < sorted->end );
    write_fencepost(sorted);    // fencepost in new sorted binbag should still be intact
    memcpy((void*)binbag_begin_iterator(sorted), (const void*)binbag_begin_iterator(bb), sizeof(char*) * N);

    // sort the index we just copied. this doesnt affect the source binbag even though we are still
    // referencing strings from it.
    qsort((char*)binbag_begin_iterator(sorted), N, sizeof(char*), compar);
    check_fencepost(bb);

    // now we copy the strings over. since our index is sorted so will the strings copied over. We update our sorted
    // binbag index with new string locations as we copy
    const char **begin = binbag_begin_iterator(sorted), **end = binbag_end_iterator(sorted);
    int n=0;
    while(begin < end) {
        const char* src = *begin;
        assert(src >= bb->begin && src<bb->end);
        *begin = sorted->tail;
        char* _p = stpcpy(sorted->tail, src); // tail is now at the next character after this string
        sorted->tail = _p + 1;
        n++;
        begin++;
    }

    return sorted;
}

binbag* binbag_distinct(binbag *bb)
{
    return binbag_sort_distinct(bb, binbag_element_sort_asc);
}

binbag* binbag_sort_distinct(binbag *bb, int (*compar)(const void*,const void*))
{
#if 0
    if(binbag_count(bb)<=1)
        return; // duh, arrays of 1 are sorted and distinct

    // sort list so non-unique strings are always next to each other
    binbag_sort(bb, compar);

    // now shrink the elements by eliminating those that are duplicates
    // we are going to build the unique set in-place. So we have two interators. They both start at the 0th element.
    // The 'u' iterator advances only as we encounter a unique element, The 'nu' iterator advanced over each element
    // unique or not. When we encounter a new unique string at 'nu' we copy it to the position at 'u', then advance 'u'.
    char **u = (char**)binbag_begin_iterator(bb);     // i points to the insertion point of the unique set
    char **e = (char**)binbag_end_iterator(bb);       // last element
    char **nu = u;                                    // j points to the next element and iterates over the older non-unique set
    char *insertion_point = *u;           // point where new strings are inserted
    while(nu < e) { // nu will always be ahead of u
        if(strcmp(*u, *nu)!=0) {
            // new unique string
            // advance u, then copy from nu => u
            u++;
            if(u!=nu) {
                char* _p = stpcpy(insertion_point, *nu);
                *u = insertion_point;
                insertion_point = _p+1;     // now immediately after the string we just inserted
            } else
                insertion_point = *(u+1);   // would be the starting point of the next string

            nu++;
        }
    }

    // now if we eliminated any duplicates we must move the string index array to the end of memory
    if(u != nu) {
        size_t new_count = (const char**)u - binbag_begin_iterator(bb);
        const char** new_begin = binbag_end_iterator(bb) - new_count;
        memmove(new_begin, binbag_begin_iterator(bb), new_count);
        bb->elements = new_begin;

    }
#endif
    return NULL;
}

const char **binbag_begin_iterator(binbag *bb) {
	return bb->elements;
}

const char **binbag_end_iterator(binbag *bb) {
	return (const char **)bb->end;
}

//const unsigned char* binbag_binary_get(binbag* bb, size_t idx, size_t* len_out);

