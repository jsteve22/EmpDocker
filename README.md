# EmpDocker

Docker container for emp-sh2pc, a garbled circuits library, based on the [repo](https://github.com/emp-toolkit/emp-sh2pc) from [emp-toolkit](https://github.com/emp-toolkit).

#### Building 

1. Clone the EmpDocker git repository by running:
    ```
    git clone https://github.com/jsteve22/EmpDocker.git
    ```

2. Enter the Framework directory: `cd EmpDocker/`

3. With docker installed, run:
    ```
    docker build -t emp-sh2pc .
    docker run -it emp-sh2pc
    ./test_bit 1 12345 123 & ./test_bit 2 12345 124
    ```
    OR
    ```
    docker build -t emp-sh2pc .
    docker run -it --name emp --mount "type=bind,source=$PWD/emp-sh2pc,target=/emp-sh2pc/" emp-sh2pc
    ```
    and re-run it with 
    ```
    docker start -i emp
    ```
    Or with apptainer, run:
    ```
    apptainer build emp.sif make.def
    apptainer build --sandbox empImg emp.sif
    apptainer shell --writable --fakeroot empImg
    cd ~
    ./test_bit 1 12345 123 & ./test_bit 2 12345 124
    ```
