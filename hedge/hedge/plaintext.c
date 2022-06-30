#include "plaintext.h"
#include "lib/debug.h"
#include "hedge_malloc.h"
#include "lib/common.h"
#include "lib/algorithm.h"
#include "ciphertext.h"
#include "math/polycore.h"
#include "math/uintcore.h"

#define PLAINTEXT_DEFAULT_DATA_SIZE 64

bool is_dec_char(char c)
{
    return c >= '0' && c <= '9';
}

int get_dec_value(char c)
{
    return c - '0';
}

int get_coeff_length(const char *poly)
{
    int length = 0;
    while (is_hex_char(*poly))
    {
        length++;
        poly++;
    }
    return length;
}

int get_coeff_power(const char *poly, int *power_length)
{
    int length = 0;
    if (*poly == '\0')
    {
        *power_length = 0;
        return 0;
    }
    if (*poly != 'x')
    {
        return -1;
    }
    poly++;
    length++;

    if (*poly != '^')
    {
        return -1;
    }
    poly++;
    length++;

    int power = 0;
    while (is_dec_char(*poly))
    {
        power *= 10;
        power += get_dec_value(*poly);
        poly++;
        length++;
    }
    *power_length = length;
    return power;
}

int get_plus(const char *poly)
{
    if (*poly == '\0')
    {
        return 0;
    }
    if (*poly++ != ' ')
    {
        return -1;
    }
    if (*poly++ != '+')
    {
        return -1;
    }
    if (*poly != ' ')
    {
        return -1;
    }
    return 3;
}


static void reserve(Plaintext* self, size_type capacity)
{
    self->data_->reserve(self->data_, capacity);
}

static void shrink_to_fit(Plaintext* self)
{
    self->data_->shrink(self->data_);
}
static void release(Plaintext* self)
{
    self->data_->release(self->data_);
}

static void resize(Plaintext* self, size_type coeff_count)
{
    self->data_ = self->data_->resize(self->data_, coeff_count);
}


static void set_zero(Plaintext* self, size_type start_coeff, size_type length)
{
    if(!length) return;

    if (start_coeff + length - 1 >= self->coeff_count(self)) {
        out_of_range("length must be non-negative and start_coeff + length - 1 must be within [0, coeff_count)");       
    }

    for (size_t i; i < length; i++) {
        self->data_->data[i] = 0;
    }
}

static bool is_ntt_form(Plaintext* self)
{
    return !EQUALS(&self->parms_id_, &parms_id_zero);
}

static parms_id_type* parms_id(Plaintext* self)
{
    return &self->parms_id_;
}
static double scale(Plaintext* self)
{
    return self->scale_;
}

static Plaintext* assign(Plaintext* self, const char* hex_poly)
{
    if (self->is_ntt_form(self)) {
        logic_error("cannot set an NTT transformed Plaintext");
    }
    if (strlen(hex_poly) > INT_MAX) {
        invalid_argument("hex_poly too long");
    }
    log_trace("%s\n", hex_poly);
    int length = (int)strlen(hex_poly);
    int assign_coeff_count = 0;

    int assign_coeff_bit_count = 0;
    int pos = 0;
    int last_power = (int)(min(max_array_count(), (int)INT_MAX));
    const char *hex_poly_ptr = hex_poly;
    while (pos < length)
    {
        // Determine length of coefficient starting at pos.
        int coeff_length = get_coeff_length(hex_poly_ptr + pos);
        if (coeff_length == 0)
        {
            invalid_argument("unable to parse hex_poly");
        }

        // Determine bit length of coefficient.
        int coeff_bit_count =
            get_hex_string_bit_count(hex_poly_ptr + pos, coeff_length);
        if (coeff_bit_count > assign_coeff_bit_count)
        {
            assign_coeff_bit_count = coeff_bit_count;
        }
        pos += coeff_length;

        // Extract power-term.
        int power_length = 0;
        int power = get_coeff_power(hex_poly_ptr + pos, &power_length);
        if (power == -1 || power >= last_power)
        {
            invalid_argument("unable to parse hex_poly");
        }
        if (assign_coeff_count == 0)
        {
            assign_coeff_count = power + 1;
        }
        pos += power_length;
        last_power = power;

        // Extract plus (unless it is the end).
        int plus_length = get_plus(hex_poly_ptr + pos);
        if (plus_length == -1)
        {
            invalid_argument("unable to parse hex_poly");
        }
        pos += plus_length;
    }

    // If string is empty, then done.
    if (assign_coeff_count == 0 || assign_coeff_bit_count == 0)
    {
        self->set_zero(self, 0, (size_type)self->data_->count);
        return self;
    }

    // Resize polynomial.
    if (assign_coeff_bit_count > BITS_PER_UINT64)
    {
        invalid_argument("hex_poly has too large coefficients");
    }
    self->resize(self, (size_t)(assign_coeff_count));

    // Populate polynomial from string.
    pos = 0;
    last_power = (int)(self->coeff_count(self));
    while (pos < length)
    {
        // Determine length of coefficient starting at pos.
        const char *coeff_start = hex_poly_ptr + pos;
        int coeff_length = get_coeff_length(coeff_start);
        pos += coeff_length;

        // Extract power-term.
        int power_length = 0;
        int power = get_coeff_power(hex_poly_ptr + pos, &power_length);
        pos += power_length;

        // Extract plus (unless it is the end).
        int plus_length = get_plus(hex_poly_ptr + pos);
        pos += plus_length;

        // Zero coefficients not set by string.
        for (int zero_power = last_power - 1; zero_power > power; --zero_power)
        {
            self->data_->data[zero_power] = 0;
        }

        // Populate coefficient.
        uint64_t *coeff_ptr = self->data_->begin(self->data_) + power;
        hex_string_to_uint((char *)coeff_start, coeff_length, (size_t)(1), coeff_ptr);
        last_power = power;
    }

    // Zero coefficients not set by string.
    for (int zero_power = last_power - 1; zero_power >= 0; --zero_power)
    {
        self->data_->data[zero_power] = 0;
    }

    return self;
}

static pt_coeff_type* data(Plaintext* self)
{
    return self->data_->begin(self->data_);
}

static pt_coeff_type* at(Plaintext* self, size_type coeff_index)
{
    if (self->coeff_count(self) == 0) {
        return nullptr;
    }
    if (coeff_index >= self->coeff_count(self)) {
        out_of_range("coeff_index must be within [0, coeff_count)");
    }
    return self->data_->begin(self->data_) + coeff_index;
}

static pt_coeff_type value_at(Plaintext* self, size_type coeff_index)
{
    return *at(self, coeff_index);
}
/**
 * Compare two plaintext
 */
static bool is_equal(Plaintext* self, const Plaintext *compare)
{
    size_t sig_coeff_count = self->significant_coeff_count(self);
    size_t sig_coeff_count_compare = compare->significant_coeff_count((Plaintext *)compare);
    bool parms_id_compare = (self->is_ntt_form(self) && compare->is_ntt_form((Plaintext *)compare)
        && EQUALS(&self->parms_id_, &compare->parms_id_)) ||
        (!self->is_ntt_form(self) && !compare->is_ntt_form((Plaintext *)compare));
    return parms_id_compare
        && (sig_coeff_count ==sig_coeff_count_compare)
        && equal((uint64_t*)self->data_->cbegin(self->data_),
            (uint64_t*)self->data_->cbegin(self->data_) + sig_coeff_count,
            (uint64_t*)compare->data_->cbegin(compare->data_),
            (uint64_t*)compare->data_->cbegin(compare->data_) + sig_coeff_count)
        && all_of(self->data_->cbegin(self->data_) + sig_coeff_count,
            self->data_->cend(self->data_), is_zero_uint64)
        && all_of(compare->data_->cbegin(compare->data_) + sig_coeff_count,
            compare->data_->cend(compare->data_), is_zero_uint64)
        && are_close(self->scale_, compare->scale_);
}

static bool is_zero(Plaintext* self)
{
    return (self->coeff_count(self) == 0) ||
        all_of(self->data_->cbegin(self->data_), self->data_->cend(self->data_),
        is_zero_uint64);
}

static size_type capacity(Plaintext* self)
{
    return self->data_->capacity;
}

static size_type coeff_count(Plaintext* self)
{
    return self->data_->size(self->data_);
}

static size_type significant_coeff_count(Plaintext* self)
{
    if (self->coeff_count(self) == 0) {
        return 0;
    }
    return get_significant_uint64_count_uint(self->data_->cbegin(self->data_),
        self->coeff_count(self));
}

static size_type nonzero_coeff_count(Plaintext* self)
{
    if (self->coeff_count(self) == 0) {
        return 0;
    }
    return get_nonzero_uint64_count_uint(self->data_->cbegin(self->data_),
        self->coeff_count(self));
}

static char* to_string(Plaintext* self)
{
    if (self->is_ntt_form(self))
        invalid_argument("cannot convert NTT transformed plaintext to string");
    return poly_to_hex_string(self->data_->cbegin(self->data_), self->coeff_count(self), 1);
}

static void save(Plaintext* self, bytestream_t* stream)
{
    stream->write(stream, (byte*)&self->parms_id_, sizeof(parms_id_type));
    stream->write(stream, (byte*)&self->scale_, sizeof(self->scale_));
    self->data_->save(self->data_, stream);

    //print_polynomial("plaintext", self->data_->data, self->data_->count, TRUE);
}

static void load(Plaintext* self, bytestream_t* stream)
{
    stream->read(stream, (byte*)&self->parms_id_, sizeof(parms_id_type));
    stream->read(stream, (byte*)&self->scale_, sizeof(self->scale_));
    self->data_ = self->data_->load(self->data_, stream);
}

Plaintext* new_Plaintext(void)
{
    Plaintext* p = hedge_malloc(sizeof(Plaintext));
    p->parms_id_ = sha3_zero_block;
    p->scale_ = 1.0;
    p->data_ = new_array(PLAINTEXT_DEFAULT_DATA_SIZE);

    p->reserve = reserve;
    p->shrink_to_fit = shrink_to_fit;
    p->release = release;
    p->resize = resize;
    p->set_zero = set_zero;
    p->data = data;
    p->at = at;
    p->value_at = value_at;
    p->is_equal = is_equal;
    p->is_zero = is_zero;
    p->capacity = capacity;
    p->coeff_count = coeff_count;
    p->significant_coeff_count = significant_coeff_count;
    p->nonzero_coeff_count = nonzero_coeff_count;
    p->to_string = to_string;
    p->save = save;
    p->load = load;
    p->is_ntt_form = is_ntt_form;
    p->parms_id = parms_id;
    p->scale = scale;

    return p;
}

void del_Plaintext(Plaintext* plaintext)
{
    hedge_free(plaintext->data_);
    hedge_free(plaintext);
}