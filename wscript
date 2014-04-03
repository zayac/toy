def options(opt):
    opt.recurse('src')


def configure(cnf):
    cnf.recurse('src')


def build(bld):
    bld.recurse('src')
