#ifndef GAME_RESULTS_HEADER
#define GAME_RESULTS_HEADER


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
     * Return raw ok result
     * Useful when the result is a shared_ptr
     */
    T getOkResultRaw() {
        return m_ok_result;
    }

    /**
     * Return nullptr if the result it not Error
     */
    E* getErrResult() {
        if ( !isError() )
            return nullptr;
        return &m_err_result;
    }

    /**
     * Return raw error result
     * Useful when the result is a shared_ptr
     */
    E getErrResultRaw() {
        return m_err_result;
    }
    
};


/*
 * Default error type to return using Result<T, E> by Game class
 * methods.
 * example usage:
 * ~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
 * return Result<VoidResult, GameError>::Err(GameError("Error message here."))
 * ~~~~~~~~~~~~~~~~~~~~~~~~
 */
#include <string>
class GameError {
public:
    std::string err_message;

    GameError() = default;

    GameError(
            std::string err_message
            )
        :
            err_message(err_message)
    { }
};

/*
 * Place holder to denote `void` type of return when using `Result<T, E>`
 * example usage:
 * ~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
 * return Result<VoidResult, GameError>::Ok(VoidResult::empty)
 * ~~~~~~~~~~~~~~~~~~~~~~~~
 */
class VoidResult {
public:
    static VoidResult empty;
};


/*
 * Default Void Result return value used by the game engine.
 */
#define Void Result<VoidResult, GameError>

/*
 * Default Ok value for typical Result<VoidResult, GameError> result type used
 * by the game engine.
 */
#define ResultOK Result<VoidResult, GameError>::Ok(VoidResult::empty)

/*
 * Default Error value for typical Result<VoidResult, GameError> result type
 * used by the game engine.
 */
#define ResultError(err_message) Result<VoidResult, GameError>::Err(GameError(err_message))


#endif
