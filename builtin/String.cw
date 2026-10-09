# Built-in String class. Literal creation is compiler-owned; no public constructor.
class String {
    private:
    buffer _bytes

    protected:
    String() {
        _bytes = buffer(0)
    }

    public:
    void append(String other)
    bool equals(String other)
    int toInt()
    unsigned toUnsigned()
    float toFloat()

    unsigned byteAt(int index)
    String slice(int first, int last)
    void insert(int index, String other)
    void removeRange(int first, int last)
    void clear()
    int find(String needle)
    bool contains(String needle)
    bool startsWith(String prefix)
    bool endsWith(String suffix)
    list split(String separator)
    String replace(String target, String replacement)
    String trim()
    String strip()
    String toUpper()
    String toLower()
}

void String.append(String other) from "cwhip_string_append"
bool String.equals(String other) from "cwhip_string_equals"
int String.toInt() from "cwhip_string_method_to_int"
unsigned String.toUnsigned() from "cwhip_string_method_to_unsigned"
float String.toFloat() from "cwhip_string_method_to_float"
unsigned String.byteAt(int index) from "cwhip_string_method_byte_at"
String String.slice(int first, int last) from "cwhip_string_method_slice"
void String.insert(int index, String other) from "cwhip_string_method_insert"
void String.removeRange(int first, int last) from "cwhip_string_method_remove_range"
void String.clear() from "cwhip_string_method_clear"
int String.find(String needle) from "cwhip_string_method_find"
bool String.contains(String needle) from "cwhip_string_method_contains"
bool String.startsWith(String prefix) from "cwhip_string_method_starts_with"
bool String.endsWith(String suffix) from "cwhip_string_method_ends_with"
list String.split(String separator) from "cwhip_string_method_split"
String String.replace(String target, String replacement) from "cwhip_string_method_replace"
String String.trim() from "cwhip_string_method_trim"
String String.strip() from "cwhip_string_method_trim"
String String.toUpper() from "cwhip_string_method_to_upper"
String String.toLower() from "cwhip_string_method_to_lower"
