//
// Created by Colin MacKenzie on 5/18/17.
//

#include <cstring>
#include <stdio.h>
#include <tests.h>
#include <binbag.h>

#define SAMPLE_L66 "Contrary to popular belief, Lorem Ipsum is not simply random text."
#define SAMPLE_L97 "It has roots in a piece of classical Latin literature from 45 BC, making it over 2000 years old."
#define SAMPLE_L165 SAMPLE_L66 " " SAMPLE_L97


TEST(binbag_create)
{
    binbag* bb = binbag_create(128, 2.0);
    if(bb) binbag_free(bb);
    return (bb!=NULL) ? OK : FAIL;
}

TEST(binbag_length_capacity_on_create)
{
    binbag* bb = binbag_create(128, 2.0);
    int res = (binbag_byte_length(bb)==0 && binbag_count(bb)==0 && binbag_capacity(bb)==128) ? OK : FAIL;
    binbag_free(bb);
    return res;
}

TEST(binbag_insert_one_matches_length)
{
    int res = OK;
    size_t len1, len2;
    binbag* bb = binbag_create(128, 2.0);
    if (binbag_insert(bb, SAMPLE_L66)!=0)   // first element is 0
        res = FAIL;
    if (res==OK && (len1=strlen(SAMPLE_L66)+1) != (len2=binbag_byte_length(bb))) {
        printf("   strlen() returned %lu, binbag_byte_length() returned %lu\n", len1, len2);
        res = FAIL;
    }
    binbag_free(bb);
    return res;
}

TEST(binbag_insert_one_has_one_element)
{
    int res = OK;
    binbag* bb = binbag_create(128, 2.0);
    if (binbag_insert(bb, SAMPLE_L66)!=0)   // first element is 0
        res = FAIL;
    if (res==OK && binbag_count(bb)!=1) {
        printf("binbag count was %lu, expected 1\n", binbag_count(bb));
        res = FAIL;
    }
    binbag_free(bb);
    return res;
}

TEST(binbag_insert_one_matches_buffer)
{
    int res = OK;
    binbag* bb = binbag_create(128, 2.0);
    if (binbag_insert(bb, SAMPLE_L66)!=0)   // first element is 0
        res = FAIL;
    if (res==OK && strcmp(bb->begin, SAMPLE_L66)!=0)
        res = FAIL;
    binbag_free(bb);
    return res;
}

TEST(binbag_insert_one_matches_getter)
{
    int res = OK;
    binbag* bb = binbag_create(128, 2.0);
    if (binbag_insert(bb, SAMPLE_L66)!=0)   // first element is 0
        res = FAIL;
    if (res==OK && strcmp(binbag_get(bb, 0), SAMPLE_L66)!=0)
        res = FAIL;
    binbag_free(bb);
    return res;
}

TEST(binbag_insert_two_without_growth)
{
    int res = OK;
    binbag* bb = binbag_create(512, 2.0);
    if (binbag_insert(bb, SAMPLE_L66)!=0)   // first element is 0
        res = FAIL;
    if (res==OK && strcmp(binbag_get(bb, 0), SAMPLE_L66)!=0)
        res = FAIL;

    if (binbag_insert(bb, SAMPLE_L97)!=1)   // second element is 1
        res = FAIL;
    if (res==OK && strcmp(binbag_get(bb, 0), SAMPLE_L66)!=0)
        res = FAIL;
    if (res==OK && strcmp(binbag_get(bb, 1), SAMPLE_L97)!=0)
        res = FAIL;

    binbag_free(bb);
    return res;
}

TEST(binbag_insert_two_with_growth)
{
    int res = OK;
    binbag* bb = binbag_create(128, 2.0);
    if (binbag_insert(bb, SAMPLE_L66)!=0)   // first element is 0
        res = FAIL;
    if (res==OK && strcmp(binbag_get(bb, 0), SAMPLE_L66)!=0)
        res = FAIL;

    if (binbag_insert(bb, SAMPLE_L97)!=1)
        res = FAIL;
    if (res==OK && strcmp(binbag_get(bb, 0), SAMPLE_L66)!=0)
        res = FAIL;
    if (res==OK && strcmp(binbag_get(bb, 1), SAMPLE_L97)!=0)
        res = FAIL;

    binbag_free(bb);
    return res;
}

TEST(binbag_insert_large_one_causes_growth)
{
    int res = OK;
    size_t len1, len2;
    binbag* bb = binbag_create(128, 2.0);

    // test capacity is 128
    if(binbag_capacity(bb)!=128)
        res = FAIL;

    if(res==OK && binbag_insert(bb, SAMPLE_L165)!=0)
        res = FAIL;

    // test capacity grew
    if(res==OK && (len1=binbag_capacity(bb))<165) {
        printf("   binbag_capacity() returned %lu, expected greater than 164\n", len1);
        res = FAIL;
    }

    // test lengths match
    if (res==OK && (len1=strlen(SAMPLE_L165)+1) != (len2=binbag_byte_length(bb))) {
        printf("   strlen() returned %lu, binbag_byte_length() returned %lu\n", len1, len2);
        res = FAIL;
    }

    // test buffer is exactly the string
    if (res==OK && strcmp(bb->begin, SAMPLE_L165)!=0)
        res = FAIL;

    // test fetching element 0 returns the right string
    if (res==OK && strcmp(binbag_get(bb, 0), SAMPLE_L165)!=0)
        res = FAIL;
    binbag_free(bb);
    return res;
}

TEST(binbag_insert_keep_growing)
{
    int res = OK;
    size_t len1, len2;
    binbag* bb = binbag_create(128, 1.1);

    // test capacity is 128
    if(binbag_capacity(bb)!=128)
    res = FAIL;

    size_t count = 10, chars=0, growths=0;
    for(size_t i=0; i<count; i++) {
        size_t cap = binbag_capacity(bb);
        if (res == OK && binbag_insert(bb, SAMPLE_L165) != i)
            res = FAIL;
        chars += strlen(SAMPLE_L165)+1;

        // test lengths match
        if (res==OK && chars != (len2=binbag_byte_length(bb))) {
            printf("   strlen() returned %lu, binbag_byte_length() returned %lu\n", chars, len2);
            res = FAIL;
        }

        // test count is correct
        if (res==OK && (i+1) != (len2=binbag_count(bb))) {
            printf("   strlen() returned %lu, binbag_byte_length() returned %lu\n", chars, len2);
            res = FAIL;
        }

        // test each element matches the string
        for(size_t j=0; j<len2; j++) {
            const char* el = binbag_get(bb, j);
            if (res == OK && strcmp(el, SAMPLE_L165) != 0) {
                const char* el = binbag_get(bb, j);
                printf("   element %lu did not match the insert string after %lu inserts and %lu growths\n", j, i+1, bb->growths);
                binbag_debug_print(bb);
                res = FAIL;
            }
        }
    }

    // test capacity grew
    if(res==OK && (len1=binbag_capacity(bb))<165) {
        printf("   binbag_capacity() returned %lu, expected greater than 164\n", len1);
        res = FAIL;
    }

    binbag_free(bb);
    return res;
}

TEST(binbag_split_typical_string)
{
    const char* str = "jim,john,mary,frank,maya,julia,,matthew,david,greg,colin,kinga";
    binbag* bb = binbag_split_string(',', 0, str);
    //binbag_debug_print(bb);
    return ((bb!=NULL) && binbag_count(bb)==12) ? OK : FAIL;
}

TEST(binbag_split_empty_string)
{
    const char* str = "";
    binbag* bb = binbag_split_string(',', 0, str);
    //binbag_debug_print(bb);
    return ((bb!=NULL) && binbag_count(bb)==0) ? OK : FAIL;
}

TEST(binbag_split_singleton_string)
{
    const char* str = "jim";
    binbag* bb = binbag_split_string(',', 0, str);
    //binbag_debug_print(bb);
    return ((bb!=NULL) && binbag_count(bb)==1) ? OK : FAIL;
}

TEST(binbag_split_seperator_terminated_string)
{
    const char* str = "jim,john,mary,frank,maya,julia,,matthew,david,greg,colin,kinga,";
    binbag* bb = binbag_split_string(',', 0, str);
    //binbag_debug_print(bb);
    return ((bb!=NULL) && binbag_count(bb)==13) ? OK : FAIL;
}

TEST(binbag_split_string_and_ignore_empties)
{
    const char* str = "jim,john,,mary,frank,maya,julia,,matthew,david,greg,colin,kinga,";
    binbag* bb = binbag_split_string(',', SF_IGNORE_EMPTY, str);
    //binbag_debug_print(bb);
    return ((bb!=NULL) && binbag_count(bb)==11) ? OK : FAIL;
}

TEST(binbag_find_case1)
{
    binbag* bb = binbag_split_string(' ', 0, "jim john mary frank maya julia matthew david greg colin kinga");
    return ((bb!=NULL) && binbag_find_case(bb, "julia")==5) ? OK : FAIL;
}

TEST(binbag_find_case_neg1)
{
    binbag* bb = binbag_split_string(' ', 0, "jim john mary frank maya julia matthew david greg colin kinga");
    return ((bb!=NULL) && binbag_find_case(bb, "Julia")==-1) ? OK : FAIL;
}

TEST(binbag_find_nocase1)
{
    binbag* bb = binbag_split_string(' ', 0, "jim john mary frank maya julia matthew david greg colin kinga");
    return ((bb!=NULL) && binbag_find_nocase(bb, "Julia")==5) ? OK : FAIL;
}

TEST(binbag_find_nocase2)
{
    binbag* bb = binbag_split_string(' ', 0, "jim john mary frank maya julia matthew david greg colin kinga");
    return ((bb!=NULL) && binbag_find_nocase(bb, "Julia")==5) ? OK : FAIL;
}

TEST(binbag_find_nocase_neg1)
{
    binbag* bb = binbag_split_string(' ', 0, "jim john mary frank maya julia matthew david greg colin kinga");
    return ((bb!=NULL) && binbag_find_nocase(bb, "harry")==-1) ? OK : FAIL;
}
TEST(binbag_sort_names)
{
    const char* str = "jim,john,mary,frank,maya,julia,matthew,david,greg,colin,kinga";
    binbag* bb = binbag_split_string(',', SF_IGNORE_EMPTY, str);
    if((bb==NULL) || binbag_count(bb)!=11)
        return FAIL;
    binbag* sorted = binbag_sort(bb, binbag_element_sort_asc);
    binbag_free(bb);
    if( strcmp(binbag_get(sorted, 0), "colin")!=0 ||
        strcmp(binbag_get(sorted, 1), "david")!=0 ||
        strcmp(binbag_get(sorted, 2), "frank")!=0 ||
        strcmp(binbag_get(sorted, 3), "greg")!=0 ||
        strcmp(binbag_get(sorted, 4), "jim")!=0 ||
        strcmp(binbag_get(sorted, 5), "john")!=0 ||
        strcmp(binbag_get(sorted, 6), "julia")!=0 ||
        strcmp(binbag_get(sorted, 7), "kinga")!=0 ||
        strcmp(binbag_get(sorted, 8), "mary")!=0 ||
        strcmp(binbag_get(sorted, 9), "matthew")!=0 ||
        strcmp(binbag_get(sorted, 10), "maya")!=0)
        return FAIL;
    return ((sorted!=NULL) && binbag_count(sorted)==11) ? OK : FAIL;
}

TEST(binbag_sort_with_duplicates)
{
    const char* str = "john,jim,john,colin,mary,frank,maya,julia,matthew,david,maya,greg,colin,kinga,john";
    binbag* bb = binbag_split_string(',', SF_IGNORE_EMPTY, str);
    if((bb==NULL) || binbag_count(bb)!=15)
        return FAIL;
    binbag* sorted = binbag_sort(bb, binbag_element_sort_asc);
    binbag_free(bb);
    if( strcmp(binbag_get(sorted, 0), "colin")!=0 ||
        strcmp(binbag_get(sorted, 1), "colin")!=0 ||
        strcmp(binbag_get(sorted, 2), "david")!=0 ||
        strcmp(binbag_get(sorted, 3), "frank")!=0 ||
        strcmp(binbag_get(sorted, 4), "greg")!=0 ||
        strcmp(binbag_get(sorted, 5), "jim")!=0 ||
        strcmp(binbag_get(sorted, 6), "john")!=0 ||
        strcmp(binbag_get(sorted, 7), "john")!=0 ||
        strcmp(binbag_get(sorted, 8), "john")!=0 ||
        strcmp(binbag_get(sorted, 9), "julia")!=0 ||
        strcmp(binbag_get(sorted, 10), "kinga")!=0 ||
        strcmp(binbag_get(sorted, 11), "mary")!=0 ||
        strcmp(binbag_get(sorted, 12), "matthew")!=0 ||
        strcmp(binbag_get(sorted, 13), "maya")!=0 ||
        strcmp(binbag_get(sorted, 14), "maya")!=0)
        return FAIL;
    return ((sorted!=NULL) && binbag_count(sorted)==15) ? OK : FAIL;
}

TEST(binbag_reverse_3)
{
    const char* str = "jim,john,mary";
    binbag* bb = binbag_split_string(',', SF_IGNORE_EMPTY, str);
    if((bb==NULL) || binbag_count(bb)!=3) return FAIL;
    binbag_inplace_reverse(bb);
    if( strcmp(binbag_get(bb, 0), "mary")!=0 ||
        strcmp(binbag_get(bb, 1), "john")!=0 ||
        strcmp(binbag_get(bb, 2), "jim")!=0)
        return FAIL;
    return ((bb!=NULL) && binbag_count(bb)==3) ? OK : FAIL;
}

TEST(binbag_reverse_1)
{
    const char* str = "mary";
    binbag* bb = binbag_split_string(',', SF_IGNORE_EMPTY, str);
    if((bb==NULL) || binbag_count(bb)!=1) return FAIL;
    binbag_inplace_reverse(bb);
    if( strcmp(binbag_get(bb, 0), "mary")!=0)
        return FAIL;
    return ((bb!=NULL) && binbag_count(bb)==1) ? OK : FAIL;
}

TEST(binbag_reverse_2)
{
    const char* str = "mary,jane";
    binbag* bb = binbag_split_string(',', SF_IGNORE_EMPTY, str);
    if((bb==NULL) || binbag_count(bb)!=2) return FAIL;
    binbag_inplace_reverse(bb);
    if( strcmp(binbag_get(bb, 0), "jane")!=0 || strcmp(binbag_get(bb, 1), "mary")!=0)
        return FAIL;
    return ((bb!=NULL) && binbag_count(bb)==2) ? OK : FAIL;
}

TEST(binbag_reverse_11)
{
    const char* str = "jim,john,mary,frank,maya,julia,matthew,david,greg,colin,kinga";
    binbag* bb = binbag_split_string(',', SF_IGNORE_EMPTY, str);
    if((bb==NULL) || binbag_count(bb)!=11) return FAIL;
    binbag_inplace_reverse(bb);
    if( strcmp(binbag_get(bb, 0), "kinga")!=0 ||
        strcmp(binbag_get(bb, 1), "colin")!=0 ||
        strcmp(binbag_get(bb, 2), "greg")!=0 ||
        strcmp(binbag_get(bb, 3), "david")!=0 ||
        strcmp(binbag_get(bb, 4), "matthew")!=0 ||
        strcmp(binbag_get(bb, 5), "julia")!=0 ||
        strcmp(binbag_get(bb, 6), "maya")!=0 ||
        strcmp(binbag_get(bb, 7), "frank")!=0 ||
        strcmp(binbag_get(bb, 8), "mary")!=0 ||
        strcmp(binbag_get(bb, 9), "john")!=0 ||
        strcmp(binbag_get(bb, 10), "jim")!=0)
        return FAIL;
    return ((bb!=NULL) && binbag_count(bb)==11) ? OK : FAIL;
}