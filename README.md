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
    ./bit_test 1 12345 123 & ./bit_test 2 12345 124
    ```
