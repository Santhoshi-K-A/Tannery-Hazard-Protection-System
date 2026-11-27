import math

# 1. Reference values from our mathematical model
reference = [20, 30, 40, 50, 60]

# 2. Simulated sensor readings
sensor = [21, 32, 39, 52, 58]

# 3. Calculate the errors
errors = []

for i in range(len(reference)):
    error = sensor[i] - reference[i]
    errors.append(error)

# 4. Calculate RMSE
squared_errors = [e ** 2 for e in errors]

mean_squared_error = sum(squared_errors) / len(squared_errors)

rmse = math.sqrt(mean_squared_error)

# 5. Calculate peak reading
peak = max(sensor)

# 6. Calculate area above background
background = 20
time_interval = 1  # minute

area = sum(
    max(value - background, 0)
    for value in sensor
) * time_interval

# 7. Display results
print("Reference values:", reference)
print("Sensor readings:", sensor)
print("Errors:", errors)

print("RMSE:", round(rmse, 3))
print("Peak reading:", peak)
print("Area above background:", area)
