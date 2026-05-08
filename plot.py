from PIL import Image, ImageDraw

# napomena: promeniti ime zeljene slike koja cuva output u liniji 67

def read_points(filename):
    points = []
    with open(filename, 'r') as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            x_str, y_str = line.split()
            x = int(x_str)
            y = int(y_str)
            points.append((x, y))
    return points


def compute_bounds(points):
    xs = [p[0] for p in points]
    ys = [p[1] for p in points]

    return min(xs), max(xs), min(ys), max(ys)


def scale_points(points, bounds, img_width, img_height, margin=20):
    min_x, max_x, min_y, max_y = bounds

    range_x = max_x - min_x if max_x != min_x else 1
    range_y = max_y - min_y if max_y != min_y else 1

    scaled = []

    for x, y in points:
        nx = (x - min_x) / range_x
        ny = (y - min_y) / range_y

        px = margin + nx * (img_width - 2 * margin)
        py = margin + ny * (img_height - 2 * margin)

        py = img_height - py

        scaled.append((px, py))

    return scaled

def draw_points(points1,
                output_file="SantaMoving.png",
                img_width=1000, img_height=1000,
                point_radius=3):

    image = Image.new("RGB", (img_width, img_height), "white")
    draw = ImageDraw.Draw(image)

    # combine all points to compute scaling
    all_points = points1 + points2
    bounds = compute_bounds(all_points)

    scaled_points1 = scale_points(points1, bounds, img_width, img_height)
    scaled_points2 = scale_points(points2, bounds, img_width, img_height)

    # 1️⃣ Draw points2 FIRST (magenta)
    for x, y in scaled_points2:
        draw.ellipse(
            [
                (x - point_radius, y - point_radius),
                (x + point_radius, y + point_radius)
            ],
            fill="magenta"
        )

    # 2️⃣ Draw points1 AFTER
    colors = ["black", "red", "blue", "green"]
    l = len(scaled_points1)

    for i, (x, y) in enumerate(scaled_points1):
        draw.ellipse(
            [
                (x - point_radius, y - point_radius),
                (x + point_radius, y + point_radius)
            ],
            fill=colors[int((i / l) * 4)]
        )

    image.save(output_file)
    print(f"Image saved as {output_file}")


if __name__ == "__main__":
    filename1 = r"C:\Users\risti\OneDrive\Radna površina\SantaTracker2\points.txt"
    filename2 = r"C:\Users\risti\OneDrive\Radna površina\SantaTracker2\giftsUsed.txt"

    points1 = read_points(filename1)
    points2 = read_points(filename2)

    draw_points(points1)