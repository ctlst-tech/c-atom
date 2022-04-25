def new_file_write_call(path):
    #return create_generated_file(full_path=path, tag='None')
    file = open(path, 'w')
    def fprint(*args, **kwargs):
        print(*args, **kwargs, file=file)
    return fprint

