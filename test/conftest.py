import pytest
import os
from .py import helper


@pytest.fixture(scope='function')
def barebox(strategy, target):
    strategy.transition('barebox')
    return target.get_driver('BareboxDriver')

@pytest.fixture(scope="session")
def barebox_config(strategy, target):
    strategy.transition('barebox')
    command = target.get_driver("BareboxDriver")
    return helper.get_config(command)

def pytest_configure(config):
    if 'LG_BUILDDIR' not in os.environ:
        if 'KBUILD_OUTPUT' in os.environ:
            os.environ['LG_BUILDDIR'] = os.environ['KBUILD_OUTPUT']
        elif os.path.isdir('build'):
            os.environ['LG_BUILDDIR'] = os.path.realpath('build')
        else:
            os.environ['LG_BUILDDIR'] = os.getcwd()

    if os.environ['LG_BUILDDIR'] is not None:
        os.environ['LG_BUILDDIR'] = os.path.realpath(os.environ['LG_BUILDDIR'])
