import re
import subprocess
from packaging import version

# returns 1 if we updated a file, 0 otherwise

lbstanza_bin_nix_file = 'nix/lbstanza-bin/default.nix'

notNone = lambda r: r != None
current_version = next(filter(notNone, \
        [re.search(r'.*STANZA-VERSION = (.*)',line) \
            for line in open('compiler/stz-params.stanza')])).group(1) \
            [1:][::-1][1:][::-1].replace(' ', '.')
print(f"current version={current_version}")
ci_version = next(filter(notNone, \
        [re.search(r'^  version = "(.*)"',line) \
            for line in open(lbstanza_bin_nix_file)])).group(1)
print(f"ci version used={ci_version}")
ci_sha256 = next(filter(notNone, \
        [re.search(r'^    sha256 = "(.*)"',line) \
            for line in open(lbstanza_bin_nix_file)])).group(1)
print(f"ci sha256 used={ci_sha256}")
vci_versio = version.parse(ci_version)
v_curversion = version.parse(current_version)

def search_n_replace(s, r, file, f):
    def read_replace(file):
        with open(file, 'r') as file:
            return f(file.read())
    data = read_replace(file)
    with open(file, 'w') as file:
        file.write(data)
def str_search_n_replace(s, r, file):
    return search_n_replace(s, r, file,
            lambda d: d.replace(s, r))

def is_sri(h):
    return h[:7] == 'sha256-'
def to_empty_hash(h):
    if is_sri(h):
        return 'sha256-'+len(h[7:][::-1][1:][::-1])*'A'+'='
    else:
        return len(h)*'0'

def update_ci_sha256():
    stderr_out = subprocess.run(["nix-build", "nix", "-A", "lbstanza-bin"],
            capture_output=True).stderr.decode('utf-8')
    new_sha256 = re.search(r'got: *(.*)', stderr_out).group(1)
    if not is_sri(new_sha256):
        new_sha256 = new_sha256[7:]
    str_search_n_replace(
            to_empty_hash(ci_sha256),
            new_sha256,
            lbstanza_bin_nix_file)

if vci_versio != v_curversion:
    print("Needs Update")
    str_search_n_replace(
            ci_version,
            current_version,
            lbstanza_bin_nix_file)
    str_search_n_replace(
            ci_sha256,
            to_empty_hash(ci_sha256),
            lbstanza_bin_nix_file)
    update_ci_sha256()
    exit(1)
else:
    print("Doesn't need Update")
    exit(0)
