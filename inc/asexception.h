#pragma once
#include <exception>
#include <string>

class ASException: public std::exception {
    public:
        ASException(const std::string& what_arg) {
            m_arg = what_arg + "\n";
        }
        ASException(const std::string& what_arg, int error_code) {
            av_strerror(error_code, m_error_buffer, sizeof(m_error_buffer));
            m_arg = what_arg + " (error '" + m_error_buffer + "')\n";
        }
        ~ASException() throw() { }
        const char* what() const throw() { return m_arg.c_str(); };
    private:
        std::string m_arg;
        char m_error_buffer[255]; /* use same value as in transcode_aac example */
};
