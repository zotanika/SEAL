#include <stdio.h>
#include <hedge.h>
#include "lib/common.h"
#include "math/uintarith.h"
#include "math/uintarithsmallmod.h"
#include "math/smallmodulus.h"
#include "math/numth.h"
#include "ctest.h"
#include "lib/exceptions.h"
#include "lib/unordered_map.h"
#include <math.h>
#include "lib/list.h"
#include <malloc.h>

struct mapkey {
	unsigned long data[4];
};
typedef struct mapkey mapkey_t;
DECLARE_UNORDERED_MAP_TYPE(mapkey_t, long);
DECLARE_UNORDERED_MAP_BODY(mapkey_t, long);

TEST(list) {
	struct node *mylist;
    struct node *entry;
	unsigned long expected_value;

    list_init(&mylist);
    list_add_tail(&mylist, (void*)5);
	EXPECT_EQ((unsigned long)mylist->data, 5);
    list_add_tail(&mylist, (void*)6);
	EXPECT_EQ((unsigned long)mylist->next->data, 6);
    list_add_head(&mylist, (void*)4);
    list_add_head(&mylist, (void*)3);
    list_add_tail(&mylist, (void*)7);
    list_add_head(&mylist, (void*)2);
	EXPECT_EQ((unsigned long)mylist->data, 2);
    //print_list(mylist);

    entry = mylist;
    list_remove(&mylist, entry);
    print_list(mylist);
	EXPECT_EQ((unsigned long)mylist->data, 3);

    entry = mylist;
	
    while(entry->next)
        entry = entry->next;
	expected_value = (unsigned long)entry->prev->data;
    list_remove(&mylist, entry);
    print_list(mylist);
	EXPECT_EQ((unsigned long)list_get_tail(mylist), expected_value);

    entry = mylist->next;
	expected_value = (unsigned long)entry->next->data;
    list_remove(&mylist, entry);
    print_list(mylist);
	EXPECT_EQ((unsigned long)mylist->next->data, expected_value);

	del_list(&mylist);
}

TEST(fmod) {
	EXPECT_EQ(fmod(4.0,2.5), 1.5);
	EXPECT_EQ(fmod(1.0,2.5), 1.0);
	EXPECT_EQ(fmod(25.0,2.5), 0.0);
	EXPECT_EQ(ceil(fmod(128587.234,2.5)), ceil(2.234));
	EXPECT_EQ(ceil(fmod(0.2345, 0.11245)), ceil(0.0096));
}

TEST(unordered_map) {
	mapkey_t key1 = {12387619710211396000UL, 13665940653142098000UL, 8634026090403502000UL, 9645045647749792000UL};
	mapkey_t key2 = {6707208442456270000UL, 5903067139452475000UL, 18107344987227310000UL, 14649963876855118000UL};
	mapkey_t key3 = {8791232297928994000UL, 15637589704617832000UL, 9225901446193250000UL, 15736154415661132000UL};
	mapkey_t key4 = {0, 0, 0, 0};
	
	unordered_map(mapkey_t, long)* map = new_unordered_map(mapkey_t, long, 2);
	map->emplace(map, unordered_map_entry(mapkey_t, key1, long, (long)&key1));
	map->emplace(map, unordered_map_entry(mapkey_t, key2, long, (long)&key2));

	EXPECT_EQ((void *)map->value_at(map, key1), &key1);
	EXPECT_EQ((void *)map->value_at(map, key2), &key2);
	EXPECT_EQ(map->size, 2);

	map->emplace(map, unordered_map_entry(mapkey_t, key3, long, (long)&key3));
	EXPECT_EQ((void *)map->value_at(map, key3), &key3);
	EXPECT_EQ(map->size, 3);
	
	map->emplace(map, unordered_map_entry(mapkey_t, key4, long, (long)&key4));
	EXPECT_EQ((void *)map->value_at(map, key4), &key4);
	EXPECT_EQ(map->size, 4);

	EXPECT_EQ((void *)map->value_at(map, key2), &key2);
	EXPECT_EQ((void *)map->value_at(map, key1), &key1);

	map->delete(map);
}

TEST(prime_number) {
	EXPECT_EQ(is_prime_number(3), true);
	EXPECT_EQ(is_prime_number(5), true);
	EXPECT_EQ(is_prime_number(7), true);
	EXPECT_EQ(is_prime_number(11), true);
	EXPECT_EQ(is_prime_number(152249), true);
	EXPECT_EQ(is_prime_number(1099511480321), true);
	EXPECT_EQ(is_prime_number(1152921504606830593UL), true);
	EXPECT_EQ(is_prime_number(1099511480323), false);
}

TEST(add_safe) {
	EXPECT_EQ(add_safe(10,2,1), 10+2+1);
	EXPECT_EQ(add_safe(10,1,1,1), 10+1+1+1);
}

TEST(sub_safe) {
	EXPECT_EQ(sub_safe(10,2,2), 10-2-2);
	EXPECT_EQ(sub_safe(10,1,1,1), 10-1-1-1);
	EXPECT_EQ(sub_safe(1099511480321,1,1,1), 1099511480321-1-1-1);
}
TEST(mul_safe) {
	EXPECT_EQ(mul_safe(10,2,3), 10*2*3);
	EXPECT_EQ(mul_safe(10,2,3,4), 10*2*3*4);
}

TEST(gcd) {
	EXPECT_EQ(gcd(11, 33), 11);
	EXPECT_EQ(gcd(1099511480321, 1099511480321), 1099511480321);
}

TEST(xgcd) {
	struct tuple result;

	xgcd(11, 33, &result);
	EXPECT_EQ(result.val[0], 11);

	xgcd(1099511480321, 1099511480321, &result);
	EXPECT_EQ(result.val[0], 1099511480321);

	xgcd(8192, 2305843009211596801, &result);
	EXPECT_EQ(result.val[0], 1);

	xgcd(8192, 230584, &result);
	EXPECT_EQ(result.val[0], 8);

}

TEST(power_of_two) {
	EXPECT_EQ(get_power_of_two(HEDGE_POLY_MOD_DEGREE_MIN), 1);
	EXPECT_EQ(get_power_of_two(HEDGE_POLY_MOD_DEGREE_MAX), 15);
}

TEST(significant_bit_count) {
	uint64_t val_uint192[3] = {0, 0, 1};
	EXPECT_EQ(get_significant_bit_count_uint(val_uint192, 3), 129);
	val_uint192[1] = 1;
	val_uint192[2] = 0;
	EXPECT_EQ(get_significant_bit_count_uint(val_uint192, 3), 65);
	EXPECT_EQ(get_significant_bit_count(1099511480321), 40);
	EXPECT_EQ(get_significant_bit_count(105401), 17);
}

TEST(add_uint_uint) {
    uint64_t difference[3] = {0, 2305707219494109184UL, 18446744073709551615UL};
    uint64_t numerator[3] = {0, 2305702271725338624UL, 0};
    uint64_t result[3] = {0, 0, 0};
    add_uint_uint(difference, numerator, 3, result);
    EXPECT_EQ(result[0], 0);
    EXPECT_EQ(result[1], 4611409491219447808UL); 
}

TEST(divide_uint192_internal) {
    uint64_t denominator = 1099511480321UL;
    uint64_t numerator[3] = {0, 0, 1};
    uint64_t quotient[3] = {0, 0, 0};
    size_t uint64_count = 3;

    int numerator_bits = get_significant_bit_count_uint(numerator, uint64_count);
    int denominator_bits = get_significant_bit_count(denominator);

    EXPECT_TRUE(numerator_bits > denominator_bits);

    uint64_count = (size_t)(divide_round_up(numerator_bits, BITS_PER_UINT64));

    uint64_t shifted_denominator[3] = {denominator, 0, 0};
    uint64_t difference[3] = {0, 0, 0};
    int denominator_shift = numerator_bits - denominator_bits;

    left_shift_uint192(shifted_denominator, denominator_shift, shifted_denominator);
    denominator_bits += denominator_shift;

    int remaining_shifts = denominator_shift;
	int loop = 0;
    while (numerator_bits == denominator_bits) {
		loop++;
		//printf("[%d] before sub\n", loop);
		//printarray(difference);
        if (sub_uint_uint(numerator, shifted_denominator, uint64_count, difference)) {
            if (remaining_shifts == 0) {
                break;
            }
			//printf("[%d] after sub\n", loop);
			//printarray(difference);
			//printarray(numerator);
            add_uint_uint(difference, numerator, uint64_count, difference);
			//printf("[%d] after add\n", loop);
			//printarray(difference);
            left_shift_uint192(quotient, 1, quotient);
            remaining_shifts--;
        }
        quotient[0] |= 1;

        numerator_bits = get_significant_bit_count_uint(difference, uint64_count);
        int numerator_shift = denominator_bits - numerator_bits;
        if (numerator_shift > remaining_shifts) {
            numerator_shift = remaining_shifts;
        }

        if (numerator_bits > 0) {
            left_shift_uint192(difference, numerator_shift, numerator);
            numerator_bits += numerator_shift;
        } else {
            set_zero_uint(uint64_count, numerator);
        }

        left_shift_uint192(quotient, numerator_shift, quotient);
        remaining_shifts -= numerator_shift;
    }

    if (numerator_bits > 0) {
        right_shift_uint192(numerator, denominator_shift, numerator);
    }

    //printarray(quotient);
}

TEST(left_shift_uint192) {
	uint64_t numerator[3] = {0, 0, 1};
	uint64_t denominator = 1099511480321;
	size_t numerator_bits = get_significant_bit_count_uint(numerator, 3);
	size_t denominator_bits = get_significant_bit_count(denominator);

	uint64_t shifted_denominator[3] = {denominator,0,0};
	int denominator_shift = numerator_bits - denominator_bits;
	left_shift_uint192(shifted_denominator, denominator_shift, shifted_denominator);
	EXPECT_EQ(shifted_denominator[0], 0);
	EXPECT_EQ(shifted_denominator[1], 18446739125940781056UL);
	EXPECT_EQ(shifted_denominator[2], 1);
}


TEST(divide_round_up) {
	EXPECT_EQ(divide_round_up(129, 64), 3);
	EXPECT_EQ(divide_round_up(128, 64), 2);
	EXPECT_EQ(divide_round_up(127, 64), 2);
	EXPECT_EQ(divide_round_up(65, 64), 2);
}

TEST(Zmodulus_large_prime) {
	const uint64_t prime = 1099511480321;
	Zmodulus* modulus = new_Zmodulus(prime);
	uint64_t* const_ratio = modulus->const_ratio;
	//printf("@@@ const_ratio = {%lu %lu %lu}\n", const_ratio[0], const_ratio[1], const_ratio[2]);
	EXPECT_EQ(const_ratio[0], 4611410109653542128UL);
	EXPECT_EQ(const_ratio[1], 16777218UL);
	EXPECT_EQ(const_ratio[2], 1003538651920UL);
	del_Zmodulus(modulus);
}

TEST(Zmodulus) {
	const uint64_t prime = 105401;
	Zmodulus* modulus = new_Zmodulus(prime);
	uint64_t* const_ratio = modulus->const_ratio;
	//printf("@@@ const_ratio = {%lu %lu %lu}\n", const_ratio[0], const_ratio[1], const_ratio[2]);
	EXPECT_EQ(const_ratio[0], 1910637518170483913UL);
	EXPECT_EQ(const_ratio[1], 175014886706099UL);
	EXPECT_EQ(const_ratio[2], 77759);
	del_Zmodulus(modulus);
}

TEST(swap) {
	uint64_t o1, o2, v1, v2;
	o1 = v1 = 1;
	o2 = v2 = 2;
	swap_uint64(&v1, &v2);
	EXPECT_EQ(v1, o2);
	EXPECT_EQ(v2, o1);
}

TEST(barrett_reduce) {
	uint64_t num[2] = {1000000, 0};
	uint64_t prime = 106501;
	Zmodulus* modulus = new_Zmodulus(prime);
	EXPECT_EQ(barrett_reduce_128(num, modulus), num[0]%prime);
	
	num[0] = 0xed97d30f83258b4c;
	num[1] = 0x31e10;
	uint64_t modval = 1099511480321;
	uint64_t answer = 102273544150;
	set_zmodulus_with(modulus, modval);

	uint64_t result = barrett_reduce_128(num, modulus);
	//printf("@@ barrett_reduce_128(0x%lx%lx, %lu) = %lu\n", num[1],num[0], modval, result);
	EXPECT_EQ(barrett_reduce_128(num, modulus),answer);
	del_Zmodulus(modulus);
}

TEST(exponentiate_uint_smallmod) {
	uint64_t operand = 274861424638;
	uint64_t exponent = 13711493806682;
	Zmodulus* modulus = new_Zmodulus(1099511480321UL);
	uint64_t result = exponentiate_uint_smallmod(operand, exponent, modulus);
	//printf("exponentiate_uint_smallmod(%lu,%lu,%lu)=%lu\n", operand, exponent, modulus->value_, result);
	EXPECT_EQ(result<modulus->value, true);
	del_Zmodulus(modulus);
}

TEST(divide_uint912_uint64_inplace) {
	uint64_t prime_num = 1099511480321UL;
	uint64_t numerator[3] = { 0, 0, 1 };
	uint64_t quotient[3] = { 0, 0, 0 };

	divide_uint192_uint64_inplace(numerator, prime_num, quotient);
	
	EXPECT_EQ(quotient[0], 4611410109653542128);
	EXPECT_EQ(quotient[1], 16777218);
	EXPECT_EQ(quotient[2], 0);
}

TEST(add_and_sub_uint_uint) {
	uint64_t numerator[3] = {0, 0, 1};
	uint64_t denominator = 1099511480321;
	uint64_t shifted_denominator[3] = {0, 18446739125940781056UL, 1};
	uint64_t difference[3] = {0,0,0};

	sub_uint_uint(numerator, shifted_denominator, 3, difference);

	EXPECT_EQ(difference[0], 0);
	EXPECT_EQ(difference[1], 4947768770560);
	EXPECT_EQ(difference[2], 18446744073709551615UL);

	add_uint_uint(difference, numerator, 3, difference);

	EXPECT_EQ(difference[0], 0);
	EXPECT_EQ(difference[1], 4947768770560);
	EXPECT_EQ(difference[2], 0);
}

TEST(bytestream) {
	bytestream_t *stream = new_bytestream(0);
	char string1[] = "1234567890";
	char string2[] = "abcdefghijklmnopqrstuvwxyz";
	FILE* file;

	stream->write(stream, string1, sizeof(string1));
	stream->write(stream, string2, sizeof(string2));
	
	file = fopen("test_bytestream.bin", "wb+");
	fwrite(stream->data, 1, stream->size, file);
	fclose(file);

	file = fopen("test_bytestream.bin", "rb");
	char * content = malloc(100);
	size_t read_size = fread(content, 1, 100, file);
	
	EXPECT_EQ(read_size, sizeof(string1) + sizeof(string2));
	EXPECT_EQ(strcmp(string1, content), 0);
	EXPECT_EQ(strcmp(string2, content + sizeof(string1)), 0);

	free(content);
	del_bytestream(stream);
}

#if 0
TEST(hedge_encode) {
	hedge_config_t config;
	config.poly_modulus_degree = 8192;
	config.scale = 40.0;

	FILE* file;
	size_t size;
	hedge_t* hedge = hedge_create(&config);

	double value[1] = {7.0};

	hedge_plaintext_t* plaintext = hedge_encode(hedge, value, 1);
	file = fopen("plaintext.bin", "wb+");
	size = fwrite(plaintext->data, 1, plaintext->size, file);
	fclose(file);

	// OPEN "plaintext.bin" in SEAL and make sure it decodes correctly

	hedge_double_array_t* result = hedge_decode(hedge, plaintext);

	hedge_shred(hedge);
	EXPECT_FLOAT_EQ((float)result->data[0], (float)value[0]);
}
#endif

TEST(hedge) {
	int i;
	hedge_config_t config;
	config.poly_modulus_degree = 8192;
	config.scale = 40.0;
	hedge_t* hedge = hedge_create(&config);

	FILE* file;
	size_t size;
	
	file = fopen("pubkey.bin", "wb+");
	EXPECT_EQ(!!file, TRUE);
	size = fwrite(hedge->pubkey->data, 1, hedge->pubkey->size, file);
	EXPECT_EQ(size, hedge->pubkey->size);
	fclose(file);

	file = fopen("seckey.bin", "wb+");
	EXPECT_EQ(!!file, TRUE);
	size = fwrite(hedge->seckey->data, 1, hedge->seckey->size, file);
	EXPECT_EQ(size, hedge->seckey->size);
	fclose(file);

	file = fopen("relinkeys.bin", "wb+");
	EXPECT_EQ(!!file, TRUE);
	size = fwrite(hedge->relinkeys->data, 1, hedge->relinkeys->size, file);
	EXPECT_EQ(size, hedge->relinkeys->size);
	fclose(file);

	double value[] = {7.0, 11.0, 113.5, 2.3, 4.1};
	size_t vsize = sizeof(value)/sizeof(double);
#if 1
	hedge_plaintext_t* plaintext = hedge_encode(hedge, value, vsize);
	file = fopen("plaintext.bin", "wb+");
	size = fwrite(plaintext->data, 1, plaintext->size, file);
	fclose(file);
	free(plaintext);
#endif

	hedge_ciphertext_t *ciphertext = hedge_encrypt(hedge, value, vsize);
	file = fopen("ciphertext.bin", "wb+");
	size = fwrite(ciphertext->data, 1, ciphertext->size, file);
	fclose(file);
	hedge_double_array_t *ret = hedge_decrypt(hedge, ciphertext);

	for (i = 0; i < vsize; i++) {
		EXPECT_FLOAT_EQ((float)ret->data[i], (float)value[i]);
	}
	hedge_shred(hedge);
}

int main(int argc, const char* argv[])
{
	size_t i;

	set_exception_handler();
	
	printf("::hedge::\n");
	printf("%s\n", argv[0]);
	printf("bits per uint64_t : %lu\n", sizeof(uint64_t) * 8);

	RUN_ALL_TESTS();

	hedge_mem_print_info("memleak.txt");
	return 0;
};
