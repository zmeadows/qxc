
// static int char_index(char c) {
//     if (c >= 'a' && c <= 'z') {
//         return c - 'a';
//     } else if (c >= 'A' && c <= 'Z') {
//         return 26 + c - 'A';
//     } else if (c >= '0' && c <= '9') {
//         return 2*26 + c - '0';
//     } else {
//         return -1;
//     }
// }
//
// bool qxc_string_set_contains(struct qxc_string_set* set, const char* str) {
//     size_t idx = char_index(*str);
//     if (idx == -1) return false;
//
//     struct qxc_string_set_node* node = set->children[idx];
//
//     while (1) {
//         if (!node->children.exists) return false;
//
//         size_t idx = char_index(*str);
//         if (idx == -1) return false;
//
//         size_t idx = char_index(str[0]);
//         if (idx == -1) return false;
//         str++;
//     }
// }
//
// void qxc_string_set_add(struct qxc_string_set* set, const char* str) {
//     const size_t len = strlen(str);
//     for (size_t i = 0; i < len; i++) {
//
//     }
//     struct qxc_string_set_node* node;
//     while (next_char != '\0') {
//         node = &node->children[
//     }
// }
