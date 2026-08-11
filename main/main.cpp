void on_input_complete(const char* text) {
    char command_buffer[BUF_SIZE];

    snprintf(command_buffer, sizeof(command_buffer), "%s", text);

    trim(text11, sizeof(text11), command_buffer);

    if (connState == ConnectionState::Connected) {
        // Always send a newline so bash re-displays its prompt,
        // even when the user pressed Enter on an empty line.
        if (strlen(text11) > 0) {
            ipts.write(text11, strlen(text11));
        }
        ipts.write("\n", 1);
    } else {
        // Not connected: only execute locally if there is actual input.
        if (strlen(text11) != 0) {
            ParsedCommand cmd = parse_command(text11);
            execute_command(cmd);
        }
    }

    t2input.free_buffer();
}
