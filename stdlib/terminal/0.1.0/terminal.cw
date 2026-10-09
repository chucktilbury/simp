namespace Terminal {
    class Terminal {
        bool isInteractive()
        bool isOutputInteractive()
        int columns()
        int rows()
        bool supportsColor()
    }

    bool Terminal.isInteractive() from "cwhip_terminal_stdin_interactive"
    bool Terminal.isOutputInteractive() from "cwhip_terminal_stdout_interactive"
    int Terminal.columns() from "cwhip_terminal_columns"
    int Terminal.rows() from "cwhip_terminal_rows"
    bool Terminal.supportsColor() from "cwhip_terminal_supports_color"
}
