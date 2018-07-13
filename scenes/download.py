from __future__ import print_function

import os
import sys
import requests
import zipfile
import tarfile
from collections import OrderedDict

scene_list = OrderedDict([
    ('cbox', 'https://www.mitsuba-renderer.org/scenes/cbox.zip'),
    ('cbox_gloss', 'https://www.dropbox.com/s/g60176ihutoa44k/cbox_gloss.tar.gz?dl=1'),
    ('rt4', 'https://www.dropbox.com/s/s41pt3togx4zuk5/rt4.tar.gz?dl=1'),
    ('rt5', 'https://www.dropbox.com/s/21zaqm0080xj3gv/rt5.tar.gz?dl=1'),
])

def get_file_from_url(url):
    temp = url.split('/')[-1]
    length = temp.rfind('?')
    if length == -1:
        return temp
    else:
        return temp[:length]

def bar_string(rate):
    length = 40
    if rate == 100.0:
        return '=' * length
    else:
        num = int(rate / (100.0 / length))
        return ('=' * num) + '>' + (' ' * (length - num - 1))

def download(url):
    filename = get_file_from_url(url)
    r = requests.get(url, stream=True)

    size_total = int(r.headers['content-length'])
    size_get = 0

    chunk_size = 1024
    with open(filename, 'wb') as f:
        for chunk in r.iter_content(chunk_size=chunk_size):
            if chunk:
                f.write(chunk)
                f.flush()
                size_get += chunk_size

                rate = min(100.0, 100.0 * size_get / size_total)
                print('[ %6.2f %% ] [ %s ]' % (rate, bar_string(rate)), end='\r')
        print('\nDownload finished!!')
        return filename

    return None

def unarchive(filename):
    _, ext = os.path.splitext(filename)
    try:
        if ext == '.gz' or ext == '.tar':
            tar = tarfile.open(filename)
            tar.extractall()
            tar.close()
        elif ext == '.zip':
            zip_file = zipfile.ZipFile(filename)
            zip_file.extractall()
            zip_file.close()
        else:
            raise Exception('Unknown extension: %s' % ext)

    except e:
        raise e


    print('File is unarchived!!')


def process(url):
    filename = download(url)
    unarchive(filename)
    os.remove(filename)

def main():
    items = [(k, v) for k, v in scene_list.items()]
    for i, p in enumerate(items):
        print('[%d] %s: %s' % (i + 1, p[0], p[1]))

    select = -1
    while True:
        try:
            choise = input('Choose scene to download: ')
            select = int(choise)
            if select >= 1 and select <= len(items):
                break
            else:
                print('[ERROR] The number you input is invalid!')
        except Exception as e:
            print('[ERROR] Please input a number!')

    process(items[select - 1][1])

if __name__ == '__main__':
    main()
