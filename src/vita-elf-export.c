#include <stdio.h>
#include <jansson.h>

#include "vita-export.h"


vita_export_t *vita_exports_load(const char *filename, const char *elf, int verbose);
vita_export_t *vita_exports_loads(FILE *text, const char *elf, int verbose);
vita_export_t *vita_export_generate_default(const char *elf);
void vita_exports_free(vita_export_t *exp);

static void show_usage(void)
{
	fprintf(stderr, "Usage: vita-elf-export elf exports imports\n\n"
					"elf: path to the elf produced by the toolchain to be used by vita-elf-create\n"
					"exports: path to the yaml file specifying the module information and exports\n"
					"json: path to write the import json generated by this tool\n");
}

json_t *pack_export_symbols(vita_export_symbol **symbols, size_t symbol_n)
{
	json_t *json_symbols = json_object();
	
	for (int i = 0; i < symbol_n; ++i)
	{
		json_object_set_new(json_symbols, symbols[i]->name, json_integer(symbols[i]->nid));
	}
	
	return json_symbols;
}

int main(int argc, char *argv[])
{
	if (argc != 4)
	{
		show_usage();
		return EXIT_FAILURE;
	}
	
	const char *elf_path = argv[1];
	const char *export_path = argv[2];
	const char *import_path = argv[3];
	
	// load our exports
	vita_export_t *exports = vita_exports_load(export_path, elf_path, 0);
	
	if (!exports)
		return EXIT_FAILURE;
	
	json_t *libraries = json_object();
	
	for (int i = 0; i < exports->module_n; ++i)
	{
		vita_library_export *lib = exports->modules[i];
		
		json_t *functions = pack_export_symbols(lib->functions, lib->function_n);
		json_t *variables = pack_export_symbols(lib->variables, lib->variable_n);
		
		// this will take ownership of 'functions' and 'variables'
		json_t *json_lib = json_pack("{sIsbsoso}", "nid", (json_int_t)lib->nid, "kernel", 0, "functions", functions, "variables", variables);
		json_object_set_new(libraries, lib->name, json_lib);
	}
	
	json_t *module = json_pack("{sIso}", "nid", (json_int_t)exports->nid, "modules", libraries);
	json_t *root = json_pack("{so}", exports->name, module);
	
	char *json = json_dumps(root, JSON_INDENT(4));
	
	FILE *fp = fopen(import_path, "w");
	
	if (!fp)
	{
		// TODO: handle this
		fprintf(stderr, "could not open '%s' for writing\n", import_path);
		return EXIT_FAILURE;
	}
	
	fwrite(json, strlen(json), 1, fp);
	fclose(fp);
	
	// TODO: free exports, free json
	return 0;
}