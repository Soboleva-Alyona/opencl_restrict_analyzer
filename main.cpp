#include "clang/Frontend/FrontendActions.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "llvm/Support/CommandLine.h"

#include "lib/analyzer/core/analyzer_parameters.h"
#include "lib/analyzer/core/pseudocl.h"
#include "lib/analyzer/frontend/analyzer.h"

using namespace clang::tooling;

// Apply a custom category to all command-line options so that they are the
// only ones displayed.
static llvm::cl::OptionCategory MyToolCategory("my-tool options");

// CommonOptionsParser declares HelpMessage with a description of the common
// command-line options related to the compilation database and input files.
// It's nice to have this help message in all tools.
static llvm::cl::extrahelp CommonHelp(CommonOptionsParser::HelpMessage);

// A help message for this specific tool can be added afterwards.
static llvm::cl::extrahelp MoreHelp("\nMore help text...\n");

int main(int argc, const char **argv) {
    clsma::analyzer analyzer("/mnt/d/Users/Rhaza/CLionProjects/opencl_restrict_analyzer/test/resources/test_simple.cl");
    const size_t global_size = 16;
    const size_t local_size = 1;
    clsma::pseudocl_mem b1 = clsma::pseudocl_create_buffer(64);
    clsma::pseudocl_mem b2 = clsma::pseudocl_create_buffer(64);
    clsma::pseudocl_mem b3 = clsma::pseudocl_create_buffer(64);
    size_t arg_sizes[3] = { sizeof(clsma::pseudocl_mem), sizeof(clsma::pseudocl_mem), sizeof(clsma::pseudocl_mem) };
    void* arg_values[3] = {&b1, &b2, &b3};
    //try {
        analyzer.analyze(clsma::analyzer::checks::address | clsma::analyzer::checks::restrict, "vecadd", 1, &global_size, &local_size, 3, arg_sizes, arg_values);
    clsma::pseudocl_release_mem_object(b3);
    clsma::pseudocl_release_mem_object(b2);
    clsma::pseudocl_release_mem_object(b1);
    //} catch (z3::exception& ex) {
    //    std::cerr << "z3::exception: " << ex.what() << std::endl;
    //}
    /*auto ExpectedParser = CommonOptionsParser::create(argc, argv, MyToolCategory);
    auto ast = clang::tooling::buildASTFromCode("__kernel void vecadd(__global int* a, __global int* b, __global int* c) {\n"
                                                "    const int idx = get_global_id(0);\n"
                                                "    c[idx] = a[idx] + b[idx];\n"
                                                "}");
    if (!ExpectedParser) {
        // Fail gracefully for unsupported options.
        llvm::errs() << ExpectedParser.takeError();
        return 1;
    }
    CommonOptionsParser& OptionsParser = ExpectedParser.get();
    ClangTool Tool(OptionsParser.getCompilations(),
                   OptionsParser.getSourcePathList());
    return Tool.run(newFrontendActionFactory<clang::SyntaxOnlyAction>().get());*/
    return 0;
}
