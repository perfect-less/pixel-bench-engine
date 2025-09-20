#include "pixbench/utils/utils.h"
#include "pixbench/utils/results.h"
#include <string>

#define CATCH_CONFIG_MAIN
#include "catch2/catch.hpp"


class DummyDataType {
public:
    double data{ 0 };
};

Result<DummyDataType, std::string> dummyFunction(
        bool success = false
        ) {
    if (success) {
        DummyDataType ok_data;
        ok_data.data = 10;
        return Result<DummyDataType, std::string>::Ok(ok_data);
    }

    return Result<DummyDataType, std::string>::Err(
            "dummyFunction set to fail"
            );
}


TEST_CASE("utils result type test", "[utilities]") {

    // 'Ok' test
    auto result_expect_ok = dummyFunction(true);
    
    REQUIRE(result_expect_ok.isOk());
    REQUIRE(!result_expect_ok.isError());
    REQUIRE(result_expect_ok.getOkResult() != nullptr);
    REQUIRE(result_expect_ok.getOkResult()->data == 10);

    
    // 'Err' test
    auto result_expect_err = dummyFunction(false);
    
    REQUIRE(!result_expect_err.isOk());
    REQUIRE(result_expect_err.isError());
    REQUIRE(result_expect_err.getErrResult() != nullptr);
    
};
