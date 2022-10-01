def new_file_write_call(path):
    #return create_generated_file(full_path=path, tag='None')
    file = open(path, 'w')
    def fprint(*args, **kwargs):
        eof = False
        try:
            eof = kwargs['close_file']
            del kwargs['close_file']
        except:
            pass

        print(*args, **kwargs, file=file, flush=eof)
    return fprint

