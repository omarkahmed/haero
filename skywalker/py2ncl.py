# This script translates the output data in the given file to a text format
# usable by the NCAR Command Language (NCL). For more information about NCL, see
# https://www.ncl.ucar.edu.

import os.path, sys

def write_ncl_var(ncl, name, var):
    if isinstance(var, float):
        ncl.write('%s = %e\n'%var)
    else:
        for v in var:
            ncl.write('    %e,\\\n'%v)
        ncl.write('/)\n')

def translate_atmosphere_input(atmosphere, ncl):
    for item_name in [x for x in dir(atmosphere) if '_' not in x]:
        var_name = 'atm_%s'%item_name
        var = getattr(atmosphere, item_name)
        write_ncl_var(ncl, var_name, var)

def translate_aerosols_input(aerosols, ncl):
    for item_name in [x for x in dir(aerosols) if '_' not in x]:
        var_name = 'aero_in_%s'%item_name
        var = getattr(aerosols, item_name)
        write_ncl_var(ncl, var_name, var)

def translate_gases_input(gases, ncl):
    for item_name in [x for x in dir(gases) if '_' not in x]:
        var_name = 'gases_in_%s'%item_name
        var = getattr(gases, item_name)
        write_ncl_var(ncl, var_name, var)

def translate_user_input(user, ncl):
    for item_name in [x for x in dir(user) if '_' not in x]:
        var_name = 'user_%s'%item_name
        var = getattr(user, item_name)
        write_ncl_var(ncl, var_name, var)

def translate_aerosols_output(aerosols, ncl):
    for item_name in [x for x in dir(gases) if '_' not in x]:
        var_name = 'aero_out_%s'%item_name
        var = getattr(aerosols, item_name)
        write_ncl_var(ncl, var_name, var)

def translate_gases_output(gases, ncl):
    for item_name in [x for x in dir(gases) if '_' not in x]:
        var_name = 'gases_out_%s'%item_name
        var = getattr(gases, item_name)
        write_ncl_var(ncl, var_name, var)

def translate_metrics_output(metrics, ncl):
    for item_name in [x for x in dir(metrics) if '_' not in x]:
        var_name = 'metrics_%s'%item_name
        var = getattr(metrics, item_name)
        write_ncl_var(ncl, var_name, var)

def translate_module(mod, ncl):
    symbols = dir(mod)
    if 'input' in symbols:
        inp = mod.input
        if 'atmosphere' in dir(inp):
            translate_atmosphere_input(inp.atmosphere, ncl)
        if 'aerosols' in dir(inp):
            translate_aerosols_input(inp.aerosols, ncl)
        if 'gases' in dir(inp):
            translate_gases_input(inp.gases, ncl)
        if 'user' in dir(inp):
            translate_user_input(inp.user, ncl)

    if 'output' in symbols:
        outp = mod.output
        if 'aerosols' in dir(outp):
            translate_aerosols_output(outp.aerosols, ncl)
        if 'gases' in dir(output_obj):
            translate_gases_output(outp.gases, ncl)
        if 'metrics' in dir(output_obj):
            translate_metrics_output(outp.metrics, ncl)

if __name__ == "__main__":
    if len(sys.argv) == 1:
        usage()
        exit(0)
    else:
        py_file = sys.argv[1]
        if not os.path.exists(py_file):
            print("%s: nonexistent file: %s\n", sys.argv[0], py_file)
            exit(1)
        suffix = py_file.find('.py')
        if suffix == -1: # Not a Python module!
            print("%s: invalid Python module: %s\n"%(sys.argv[0], py_file))
            exit(1)
        sys.path.append(os.path.dirname(py_file))
        mod_name = py_file[:suffix]
        ncl_file = mod_name + ".ncl"
        ncl = open(ncl_file, 'w')

        # Import the output module.
        import importlib
        mod = importlib.import_module(mod_name)

        # Translate!
        translate_module(mod, ncl)
