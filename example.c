// ./example -h
// ./example -v -n 42 -f data.txt -b true -d 3.14
// ./example -vn 42 -fdata.txt -b1 -d2.5
// ./example -vf data.txt

#define SHOP_IMPLEMENTATION
#include "shop.h"

int main(int argc, char **argv) {
    shop_option_t options[] = {
        { .name = 'h', .require_arg = false, .info = "Show help" },
        { .name = 'v', .require_arg = false, .info = "Verbose mode" },
        { .name = 'n', .require_arg = true,  .info = "Number (int)" },
        { .name = 'f', .require_arg = true,  .info = "Filename (string)" },
        { .name = 'b', .require_arg = true,  .info = "Boolean flag" },
        { .name = 'd', .require_arg = true,  .info = "Double value" },
        SHOP_END
    };

    shop_set(options);
    shop_track(argc, argv);

    // test help
    if (shop_use('h')) {
        shop_help();
        return 0;
    }

    printf("=== Parsing Results ===\n");

    if (shop_use('v')) {
        printf("Verbose mode: ON\n");
    }

    // test number
    int number;
    if (shop_sget('n', "%d", &number)) {
        printf("Number: %d\n", number);
    }

    // test string
    const char *filename;
    if (shop_sget('f', NULL, &filename)) {
        printf("Filename: %s\n", filename);
    }

    // test flag
    bool flag;
    if (shop_sget('b', "%b", &flag)) {
        printf("Boolean flag: %s\n", flag ? "true" : "false");
    }

    // test float
    double value;
    if (shop_sget('d', "%lf", &value)) {
        printf("Double value: %.2f\n", value);
    }

    // test unknown
    if (!shop_use('x')) {
        printf("Option -x not used\n");
    }

    return 0;
}
