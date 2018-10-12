import matplotlib.pyplot as plt
import json
import argparse


def draw_sample(ax, xy, radius, draw_circles):
    dot = plt.Circle(xy, 0.05, color='black', clip_on=False)
    ax.add_artist(dot)
    if draw_circles:
        circle = plt.Circle(xy, radius, color='r', clip_on=False, fill=False)
        ax.add_artist(circle)


def draw_samples(ax, xy_list, radius, draw_circles):
    for xy in xy_list:
        draw_sample(ax, xy, radius, draw_circles)


def main(json_input_filename, image_output_filename, draw_circles):
    with open(json_input_filename, "r") as read_file:
        data = json.load(read_file)

    fig, ax = plt.subplots()  # note we must use plt.subplots, not plt.subplot

    ax.set_xlim((data["min"][0], data["max"][0]))
    ax.set_ylim((data["min"][1], data["max"][1]))
    ax.set_aspect('equal')

    xy_list = data["samples"]
    radius = data["radius"]

    draw_samples(ax, xy_list, radius, draw_circles)

    fig.savefig(image_output_filename, dpi=300)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Plot Poisson distribution.')
    parser.add_argument("--json", help="input json filename")
    parser.add_argument("--image", help="output image filename")
    parser.add_argument("--draw_circles", help="draw sample radii", default=False, action="store_true")
    args = parser.parse_args()

    main(args.json, args.image, args.draw_circles)
