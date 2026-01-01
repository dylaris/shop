// ./example -h
// ./example -v -n 42 -f data.txt -b true -d 3.14
// ./example -vn 42 -fdata.txt -b1 -d2.5
// ./example -vf data.txt

#define SHOP_IMPLEMENTATION
#include "shop.h"

int main(int argc, char **argv) {
    shop_set("vn:f:b:p:h");

    shop_desc('h', NULL, "Show this help message with detailed information about all options");
    shop_desc('v', NULL, "Enable verbose output mode for debugging purposes");
    shop_desc('n', "%d", "Number (int)");
    shop_desc('f', "%s", "Filename (string)");
    shop_desc('b', "%b", "Boolean flag");
    shop_desc('p', "%f", "Float point");

    shop_track(argc, argv);

    // test help
    if (shop_use('h')) {
        shop_help();
        return 0;
    }

    printf("=== Parsing Results ===\n");

    if (shop_use('v')) {
        shop_verbose();
    }

    // test number
    int number;
    if (shop_sget('n', 0, &number)) {
        printf("Number: %d\n", number);
    }

    // test string
    const char *filename;
    shop_foreach('f', i, &filename) {
        printf("Filename[%ld]: %s\n", i, filename);
    }

    // test flag
    bool flag;
    shop_foreach('b', i, &flag) {
        printf("Boolean flag[%ld]: %s\n", i, flag ? "true" : "false");
    }

    // test float
    float value;
    if (shop_sget('p', 0, &value)) {
        printf("Double value: %.2f\n", value);
    }

    // test unknown
    if (!shop_use('x')) {
        printf("Option -x not used\n");
    }

    shop_free();
    return 0;
}
