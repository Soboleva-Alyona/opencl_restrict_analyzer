#include "clang/Frontend/FrontendActions.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "llvm/Support/CommandLine.h"

#include "lib/analyzer/analyzer.h"

#include <z3++.h>

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

#include "OpenCL/cl.h"

int main(int argc, const char **argv) {
    clsma::analyzer analyzer("/Users/ali-al/CLionProjects/opencl-restrict-analyzer/test/resources/test_simple.cl");
    const size_t global_size = 16;
    const size_t local_size = 1;
    cl_platform_id platform_id;
    cl_device_id device_id;
    cl_context context;
    cl_int err;
    err = clGetPlatformIDs(1, &platform_id, nullptr);
    err = clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_GPU, 1, &device_id, nullptr);
    context = clCreateContext(nullptr, 1, &device_id, nullptr, nullptr, &err);
    cl_mem b1 = clCreateBuffer(context, CL_MEM_READ_ONLY, 64, nullptr, &err);
    cl_mem b2 = clCreateBuffer(context, CL_MEM_READ_ONLY, 64, nullptr, &err);
    cl_mem b3 = clCreateBuffer(context, CL_MEM_READ_WRITE, 64, nullptr, &err);
    size_t arg_sizes[3] = { sizeof(cl_mem), sizeof(cl_mem), sizeof(cl_mem) };
    void* arg_values[3] = {&b1, &b2, &b3};
    //try {
        analyzer.analyze(clsma::checks::address | clsma::checks::restrict, "vecadd", 1, &global_size, &local_size, 3, arg_sizes, arg_values);
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
