#include "dxc/dxcapi.h"
#include <emscripten/bind.h>

#include <stdio.h>
#include <iostream>
#include <locale>
#include <codecvt>
#include <string>
#include <list>


using namespace emscripten;


IDxcLibrary *pLibrary = nullptr;
IDxcCompiler *pCompiler = nullptr;

std::wstring utf8_to_utf16(const char *s)
{
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t> conversion;
	return conversion.from_bytes(s);
}

std::wstring utf8_to_utf16(const std::string& s)
{
  return utf8_to_utf16(s.c_str());
}

std::string utf16_to_utf8(const std::wstring &s)
{
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t> conversion;
	return conversion.to_bytes(s);
}

val Compile(const val& sourceCode, const val& options)
{
  val result = val::object();

  result.set("success", false);

  std::vector<std::wstring> wstrings;

  DxcDefine* pDefines = nullptr;
  size_t definesCount = 0;
  if (options.hasOwnProperty("defines"))
  {
    auto defines = options["defines"];
    definesCount = defines["length"].as<unsigned>();
    if (definesCount > 0)
    {
      pDefines = (DxcDefine*)malloc(sizeof(DxcDefine) * definesCount);
    }
    for(unsigned i = 0; i < definesCount; ++i)
    {
      std::string define = defines[i].as<std::string>();

      auto equalsIndex = define.find('=');
      if (equalsIndex != std::string::npos)
      {
        wstrings.push_back(utf8_to_utf16(define.substr(0, equalsIndex)));
        pDefines[i].Name = wstrings[wstrings.size() - 1].c_str();
        wstrings.push_back(utf8_to_utf16(define.substr(equalsIndex + 1)));
        pDefines[i].Value = wstrings[wstrings.size() - 1].c_str();;
      }
      else
      {
        wstrings.push_back(utf8_to_utf16(define.substr(0, equalsIndex)));
        pDefines[i].Name = wstrings[wstrings.size() - 1].c_str();
        pDefines[i].Value = nullptr;
      }
    }
  }

  LPCWSTR filename = L"HLSL";

  if (options.hasOwnProperty("filename"))
  {
    wstrings.push_back(utf8_to_utf16(options["filename"].as<std::string>()));
    filename = wstrings[wstrings.size() - 1].c_str();
  }

  LPCWSTR entryPoint = L"main";

  if (options.hasOwnProperty("entryPoint"))
  {
    wstrings.push_back(utf8_to_utf16(options["entryPoint"].as<std::string>()));
    entryPoint = wstrings[wstrings.size() - 1].c_str();
  }

  LPCWSTR profile = L"ps_6_4";

  if (options.hasOwnProperty("profile"))
  {
    wstrings.push_back(utf8_to_utf16(options["profile"].as<std::string>()));
    profile = wstrings[wstrings.size() - 1].c_str();
  }

  LPCWSTR* pArguments = nullptr;
  size_t argsCount = 0;
  if (options.hasOwnProperty("args"))
  {
    auto args = options["args"];
    unsigned newArgsCount = args["length"].as<unsigned>();
    pArguments = (LPCWSTR*)malloc(sizeof(LPCWSTR) * (argsCount + newArgsCount));
    for(unsigned i = 0; i < newArgsCount; ++i)
    {
      std::string str = args[i].as<std::string>();
      wstrings.push_back(utf8_to_utf16(str));
      pArguments[argsCount + i] = wstrings[wstrings.size() - 1].c_str();
    }
    argsCount += newArgsCount;
  }

  if (!pLibrary)
  {
    DxcCreateInstance(CLSID_DxcLibrary, __uuidof(IDxcLibrary), (void **)&pLibrary);
  }

  IDxcBlobEncoding *pSource;

  std::string sourceCodeStr = sourceCode.as<std::string>();
  pLibrary->CreateBlobWithEncodingFromPinned(sourceCodeStr.data(), sourceCodeStr.size(), CP_ACP, &pSource);

  if (!pCompiler)
  {
    DxcCreateInstance(CLSID_DxcCompiler, __uuidof(IDxcCompiler), (void **)&pCompiler);
  }

  IDxcOperationResult *pResult;

  pCompiler->Compile(
      pSource,
      filename,
      entryPoint,
      profile,
      pArguments,
      argsCount,
      pDefines,
      definesCount,
      nullptr,          // handler for #include directives
      &pResult);

  HRESULT status = -1;
  pResult->GetStatus(&status);

  if (status == 0)
  {
    IDxcBlob *blob = nullptr;
    pResult->GetResult(&blob);
    if (blob)
    {
      size_t bufferSize = blob->GetBufferSize();
      void *buffer = blob->GetBufferPointer();

      result.set("success", true);
      result.set("data",
        val(typed_memory_view(bufferSize, reinterpret_cast<unsigned char*>(buffer)))
      );

      blob->Release();
    }
  }

  pSource->Release();

  IDxcBlobEncoding *pPrintBlob;
  pResult->GetErrorBuffer(&pPrintBlob);

  if (pPrintBlob)
  {
    std::string error = std::string((const char*)pPrintBlob->GetBufferPointer(), pPrintBlob->GetBufferSize());
    result.set("error", error);

    pPrintBlob->Release();
  }

  if (pArguments != nullptr)
  {
    free(pArguments);
  }

  if (pDefines != nullptr)
  {
    free(pDefines);
  }

  return result;
}

EMSCRIPTEN_BINDINGS(dxcompiler)
{
  emscripten::function("Compile", &Compile);
}

