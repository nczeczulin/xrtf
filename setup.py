from setuptools import setup, find_packages, Extension

with open('README.md', 'r', encoding='utf-8') as f:
    readme = f.read()

ext_modules = [
    Extension('xrtf._xrtf',
        sources=['ext/module.c', 'ext/compression.c', 'ext/tokenizer.c'],
    )
]

setup(name='xrtf',
      description='Rich Text Format (RTF) Compression',
      author='Nick Czeczulin',
      long_description=readme,
      long_description_content_type='text/markdown',
      license='MIT',
      url='https://github.com/nczeczulin/xrtf',
      classifiers=[
        "Topic :: Communications :: Email",
        "Development Status :: 2 - Pre-Alpha",
        "Intended Audience :: Developers",
        "Natural Language :: English",
        "License :: OSI Approved :: MIT License",
        "Programming Language :: Python",
        "Programming Language :: Python :: 3",
        "Programming Language :: Python :: 3 :: Only",
        "Programming Language :: Python :: 3.6",
        "Programming Language :: Python :: 3.7",
        "Programming Language :: Python :: Implementation :: CPython",
        "Operating System :: POSIX :: Linux",
        "Operating System :: MacOS :: MacOS X",
        "Operating System :: Microsoft :: Windows"
      ],
      python_requires=">=3.6",
      packages=find_packages(where="src"),
      package_dir={"": "src"},
      ext_modules=ext_modules,
      use_scm_version=True,
      install_requires=[],
      setup_requires=['setuptools_scm', "pytest-runner"],
      tests_require=['pytest']
)
