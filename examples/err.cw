
namespace Bar {
    class Foo {

        handle fh
        strg err

        Foo() {}

        void open(strg fname, strg mode) {

            inline(handle fh, strg err, strg fname, strg mode) {
                *fh = simp_file_open(NULL, *fname, *mode);
                *err = simp_system_last_error(NULL);
            }

            if(err.length > 0) {
                print(format("error opening \"{}\": \"{}\"", fname, err))
            }
        }
    }
}

start {

    Bar.Foo f()
    f.open("foo", "r")
}