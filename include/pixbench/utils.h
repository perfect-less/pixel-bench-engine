#ifndef UTILS_HEADER
#define UTILS_HEADER


#include <SDL3/SDL_stdinc.h>
#include <sys/types.h>

void PrepareRandomGenerator();

u_int32_t GenerateRandomUInt32();

class Color {
private:
public:
    Uint8 r, g, b, a;
    Color (Uint8 r, Uint8 g, Uint8 b, Uint8 a) {
        this->r = r;
        this->g = g;
        this->b = b;
        this->a = a;
    }

    Color() : Color(0, 0, 0, 255) {}

    static Color GetWhite() {
        return Color(255, 255, 255, 255);
    }

    static Color GetBlack() {
        return Color();
    }

    static Color GetGray() {
        return Color(170, 170, 170, 255);
    }
};

// Result Type
template<typename T, typename E>
class Result {
private:
    bool m_ok{ false };
    T m_ok_result;
    E m_err_result;
public:

    Result() = default;
    
    Result(bool is_ok) : m_ok(is_ok)
    { }

    /*
     * Create Ok/Success result for Result<T, E>
     */
    static Result<T, E> Ok(
        T ok_result
        ) {
        auto result = Result<T, E>(true);
        result.m_ok_result = ok_result;
        return result;
    }

    /*
     * Create Error result for type Result<T, E>
     */
    static Result<T, E> Err(
        E err_result
        ) {
        auto result = Result<T, E>(false);
        result.m_err_result = err_result;
        return result;
    }

    /*
     * Return `true` if result is not an error
     */
    bool isOk() {
        return m_ok;
    }

    /*
     * Return `true` if result is an error
     */
    bool isError() {
        return !m_ok;
    }

    /**
     * Return nullptr if the result it not Ok
     */
    T* getOkResult() {
        if ( !isOk() )
            return nullptr;
        return &m_ok_result;
    }

    /**
     * Return nullptr if the result it not Error
     */
    E* getErrResult() {
        if ( !isError() )
            return nullptr;
        return &m_err_result;
    }
    
};

#endif
