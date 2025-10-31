# S3GBR
# Repository of the project of the S3GBR group of EPITA students.
# To use the program  :
# Put you input images in the folder data/input/
# Compile the program with the command 'make all' at the root
# Execute the compiled program with './wordsearch_solver input_image.jpg'
#                    with input_image.jpg the name of you image
#
# You can delete the executable and all the ouput files with 'make clean'
#
# To test the Proof of Concept of the neural network :
#   - Go to the nn_POC folder
#   - Compile the program using the command 'make' or 'make all'
#   - Execute the compiled program with './xor_nn {epochs} {lr} {seed}
#       with :
#           - epochs : (int) representing the number of learning cycle
#           - lr     : (double) learning rate (magnitude of the weight update)
#           - seed   : (unsigned int) representing the seed of the random generator
# By default, epochs = 10000, lr = 0.5, seed = 42
