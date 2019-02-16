#include "spirv_cpp.hpp"
#include "spirv_cross_util.hpp"
#include "spirv_glsl.hpp"
#include "spirv_hlsl.hpp"
#include "spirv_msl.hpp"
#include "spirv_parser.hpp"
#include "spirv_reflect.hpp"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <functional>
#include <limits>
#include <memory>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <emscripten/bind.h>

#include <iostream>


using namespace spv;
using namespace spirv_cross;
using namespace std;
using namespace emscripten;

static const int DEFAULT_VERSION = 200;
static const bool DEFAULT_ES = true;
static const bool DEFAULT_FLIP_VERT_Y = false;


val SPIRVtoGLSL(const val& data, val options)
{
	val result = emscripten::val::object();
	result.set("success", false);

	std::vector<uint8_t> dataVec = vecFromJSArray<uint8_t>(data);

	size_t words = int(0.5 + float(dataVec.size()) / 4.0);

	Parser spirv_parser((const uint32_t*)dataVec.data(), words);

	try
	{
		spirv_parser.parse();
	}
	catch(std::runtime_error& e)
	{
		result.set("error", e.what());
		return result;
	}

	auto parsed_ir = spirv_parser.get_parsed_ir();


	if (options.hasOwnProperty("reflect") && options["reflect"].as<bool>())
	{
		CompilerReflection compiler(parsed_ir);
		compiler.set_format("json");
		try
		{
			std::string reflection = compiler.compile();
			result.set("reflection", reflection);
		}
		catch(std::runtime_error& e)
		{
			result.set("error", e.what());
			return result;
		}
	}

	unique_ptr<CompilerGLSL> compiler(new CompilerGLSL(parsed_ir));

	CompilerGLSL::Options compilerOptions;

	compilerOptions.version = options.hasOwnProperty("version") ? options["version"].as<int>() : DEFAULT_VERSION;

	compilerOptions.es = options.hasOwnProperty("es")
    ? options["es"].as<bool>()
    : DEFAULT_ES;

	compilerOptions.vertex.flip_vert_y = options.hasOwnProperty("flip_vert_y")
    ? options["flip_vert_y"].as<bool>()
    : DEFAULT_FLIP_VERT_Y;

	compilerOptions.fragment.default_float_precision = options.hasOwnProperty("default_float_precision")
		? options["default_float_precision"].as<CompilerGLSL::Options::Precision>()
		: (CompilerGLSL::Options::Highp);

	compilerOptions.fragment.default_int_precision = options.hasOwnProperty("default_int_precision")
		? options["default_int_precision"].as<CompilerGLSL::Options::Precision>()
		: (CompilerGLSL::Options::Highp);

  compiler->set_common_options(compilerOptions);

  try
  {
    compiler->build_combined_image_samplers();

    spirv_cross_util::inherit_combined_sampler_bindings(*compiler);

    // Give the remapped combined samplers new names.
    for (auto &remap : compiler->get_combined_image_samplers())
    {
      compiler->set_name(remap.combined_id, join("SPIRV_Cross_Combined", compiler->get_name(remap.image_id),
                                                 compiler->get_name(remap.sampler_id)));
    }
  }
  catch(std::runtime_error& e)
  {
		result.set("error", e.what());
		return result;
  }

	try
	{
		std::string glsl = compiler->compile();

		result.set("glsl", glsl);

		result.set("success", true);
	}
	catch(std::runtime_error& e)
	{
		result.set("error", e.what());
		return result;
	}

	return result;
}

EMSCRIPTEN_BINDINGS(spirv)
{
  emscripten::function("SPIRVtoGLSL", &SPIRVtoGLSL);
}

