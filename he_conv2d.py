
def main():
  image = (32, 32, 3)
  filters = (3, 3, 3, 64)
  ciphertext = 2**13

  gaz = 0
  pol = 0

  # layer 0
  gazelle_layer0 = gazelle(image, filters, ciphertext, padding=1)
  polymult_layer0 = polymult(image, filters, ciphertext, padding=1)
  print(f'Layer 0')
  print(f'Gazelle mults: {gazelle_layer0}')
  print(f'Polymult mults: {polymult_layer0}')
  print()
  gaz += gazelle_layer0
  pol += polymult_layer0

  image = (32, 32, 64)
  filters = (3, 3, 64, 64)

  # layer 1
  gazelle_layer1 = gazelle(image, filters, ciphertext, padding=1) 
  polymult_layer1 = polymult(image, filters, ciphertext, padding=1) 
  print(f'Layer 1')
  print(f'Gazelle mults: {gazelle_layer1}')
  print(f'Polymult mults: {polymult_layer1}')
  print()
  gaz += gazelle_layer1
  pol += polymult_layer1

  # meanpooling
  image = (16, 16, 64)
  filters = (3, 3, 64, 64)

  # layer 2
  gazelle_layer2 = gazelle(image, filters, ciphertext, padding=1) 
  polymult_layer2 = polymult(image, filters, ciphertext, padding=1) 
  print(f'Layer 2')
  print(f'Gazelle mults: {gazelle_layer2}')
  print(f'Polymult mults: {polymult_layer2}')
  print()
  gaz += gazelle_layer2
  pol += polymult_layer2

  image = (16, 16, 64)
  filters = (3, 3, 64, 64)

  # layer 3
  gazelle_layer3 = gazelle(image, filters, ciphertext, padding=1) 
  polymult_layer3 = polymult(image, filters, ciphertext, padding=1) 
  print(f'Layer 3')
  print(f'Gazelle mults: {gazelle_layer3}')
  print(f'Polymult mults: {polymult_layer3}')
  print()
  gaz += gazelle_layer3
  pol += polymult_layer3

  # meanpooling
  image = (8, 8, 64)
  filters = (3, 3, 64, 64)

  # layer 4
  gazelle_layer4 = gazelle(image, filters, ciphertext, padding=1) 
  polymult_layer4 = polymult(image, filters, ciphertext, padding=1) 
  print(f'Layer 4')
  print(f'Gazelle mults: {gazelle_layer4}')
  print(f'Polymult mults: {polymult_layer4}')
  print()
  gaz += gazelle_layer4
  pol += polymult_layer4

  image = (8, 8, 64)
  filters = (1, 1, 64, 64)

  # layer 5
  gazelle_layer5 = gazelle(image, filters, ciphertext, padding=0) 
  polymult_layer5 = polymult(image, filters, ciphertext, padding=0) 
  print(f'Layer 5')
  print(f'Gazelle mults: {gazelle_layer5}')
  print(f'Polymult mults: {polymult_layer5}')
  print()
  gaz += gazelle_layer5
  pol += polymult_layer5


  image = (8, 8, 64)
  filters = (1, 1, 64, 16)

  # layer 6
  gazelle_layer6 = gazelle(image, filters, ciphertext, padding=0) 
  polymult_layer6 = polymult(image, filters, ciphertext, padding=0) 
  print(f'Layer 6')
  print(f'Gazelle mults: {gazelle_layer6}')
  print(f'Polymult mults: {polymult_layer6}')
  print()
  gaz += gazelle_layer6
  pol += polymult_layer6

  print()
  print(f'Gazelle mults: {gaz}')
  print(f'Polymult mults: {pol}')
  return

def gazelle(image, filters, ciphertext, padding=0, no_packing=False, debug=False):
  image_width, image_height, image_channels = image
  filter_width, filter_height, filter_in, filter_out = filters

  # image_width, image_height = image_width + padding, image_height + padding

  if no_packing:
    # no filter packing
    return filter_width * filter_height * filter_in * filter_out

  image_size = (image_width * image_height)
  images_per_ct = min(ciphertext // image_size, filter_in)

  if debug:
    print(f'image_per_ct = {images_per_ct}')
  
  return (filter_width * filter_height * filter_in * filter_out) / images_per_ct

def polymult(image, filters, ciphertext, padding=0, no_packing=False, debug=False):
  image_width, image_height, image_channels = image
  filter_width, filter_height, filter_in, filter_out = filters

  image_width, image_height = image_width + padding, image_height + padding

  if no_packing:
    # no filter packing
    return filter_in * filter_out
  
  expanded_filter_size = ((filter_width-1)*image_width) + filter_height
  image_size = (image_width * image_height)

  filters_per_ct = (ciphertext - expanded_filter_size) // (image_size)
  while (filter_out % filters_per_ct != 0):
    filters_per_ct -= 1

  if debug:
    print(f'filters_per_ct = {filters_per_ct}')

  return (filter_in * filter_out) / filters_per_ct


if __name__ == '__main__':
  main()
