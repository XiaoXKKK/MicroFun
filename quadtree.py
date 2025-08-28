class RegionQuadTreeNode:
    def __init__(self, image, x, y, size):
        self.image = image
        self.x = x
        self.y = y
        self.size = size
        self.value = None
        self.children = []

    def is_leaf(self):
        return len(self.children) == 0

    def subdivide(self):
        if self.is_leaf():
            half_size = self.size // 2
            self.children = [
                RegionQuadTreeNode(self.image, self.x, self.y, half_size),
                RegionQuadTreeNode(self.image, self.x + half_size, self.y, half_size),
                RegionQuadTreeNode(self.image, self.x, self.y + half_size, half_size),
                RegionQuadTreeNode(self.image, self.x + half_size, self.y + half_size, half_size)
            ]
            for child in self.children:
                child.build()

    def build(self):
        if self.size == 1:
            self.value = self.image[self.y][self.x]
        else:
            self.subdivide()

    def compress(self, threshold):
        if self.is_leaf():
            return
        
        if all(child.is_leaf() and child.value == self.children[0].value for child in self.children):
            self.value = self.children[0].value
            self.children = []
        else:
            for child in self.children:
                child.compress(threshold)

    def reconstruct(self, image):
        if self.is_leaf():
            for y in range(self.y, self.y + self.size):
                for x in range(self.x, self.x + self.size):
                    image[y][x] = self.value
        else:
            for child in self.children:
                child.reconstruct(image)

# 示例用法
image = [
    [0, 0, 1, 1],
    [0, 0, 1, 1],
    [1, 1, 0, 0],
    [1, 1, 0, 0]
]

tree = RegionQuadTreeNode(image, 0, 0, len(image))
tree.build()
tree.compress(threshold=1)

reconstructed_image = [[0] * len(image) for _ in range(len(image))]
tree.reconstruct(reconstructed_image)

print("Original image:")
for row in image:
    print(row)

print("Reconstructed image:")
for row in reconstructed_image:
    print(row)
