import argparse
import math
import random

def roll_dice(max_val):
    """Simulates a C-style rand() % max_val.
    Returns an integer from 0 to max_val - 1.
    """
    if max_val <= 0:
        return 0
    return random.randint(0, max_val - 1)

def calculate_analytical_probabilities(axles_per_train, defects_sorted, max_dice):
    """Calculates the exact theoretical probabilities using Binomial Distribution."""
    # 1. Calculate P_axle sequentially due to the early-break loop logic
    p_axle = 0.0
    current_clean_prob = 1.0
    
    for _, rate in defects_sorted:
        if rate > 0:
            # Threshold probability for this specific defect roll
            p_defect = (max_dice // rate) / max_dice
            # It only has a chance to trigger if all previous defects failed
            p_axle += current_clean_prob * p_defect
            current_clean_prob *= (1.0 - p_defect)
            
    p_clean = 1.0 - p_axle
    n = axles_per_train
    
    # 2. Apply Binomial Formula: P(X=k) = comb(n, k) * (p_axle^k) * (p_clean^(n-k))
    prob_0 = math.comb(n, 0) * (p_axle ** 0) * (p_clean ** n)
    prob_1 = math.comb(n, 1) * (p_axle ** 1) * (p_clean ** (n - 1))
    prob_2 = math.comb(n, 2) * (p_axle ** 2) * (p_clean ** (n - 2))
    prob_3_plus = 1.0 - (prob_0 + prob_1 + prob_2)
    prob_any = 1.0 - prob_0
    
    return {
        "p_axle": p_axle,
        "clean": prob_0 * 100,
        "defect_1": prob_1 * 100,
        "defect_2": prob_2 * 100,
        "defect_3_plus": prob_3_plus * 100,
        "any_defect": prob_any * 100
    }

def simulate_with_analytics(total_trains, axles_per_train, defects):
    # Sort defects by rate descending (highest axle rate first)
    defects_sorted = sorted(defects, key=lambda x: x[1], reverse=True)

    # Track statistics
    total_dice_rolls = 0
    total_axles_simulated = 0
    
    trains_with_1_defect = 0
    trains_with_2_defects = 0
    trains_with_3_plus_defects = 0
    
    defect_counts = {name: 0 for name, _ in defects_sorted}
    total_defects_triggered = 0

    PROBABILITY_MAX = 100000 

    # Run the math analysis first
    analytics = calculate_analytical_probabilities(axles_per_train, defects_sorted, PROBABILITY_MAX)

    # Simulate train by train
    for _ in range(total_trains):
        defects_on_this_train = 0
        
        for _ in range(axles_per_train):
            total_axles_simulated += 1
            
            for name, rate in defects_sorted:
                total_dice_rolls += 1
                threshold = PROBABILITY_MAX // rate if rate > 0 else 0
                
                if roll_dice(PROBABILITY_MAX) < threshold:
                    defect_counts[name] += 1
                    total_defects_triggered += 1
                    defects_on_this_train += 1
                    break 
                    
        if defects_on_this_train == 1:
            trains_with_1_defect += 1
        elif defects_on_this_train == 2:
            trains_with_2_defects += 1
        elif defects_on_this_train >= 3:
            trains_with_3_plus_defects += 1

    total_defective_trains = trains_with_1_defect + trains_with_2_defects + trains_with_3_plus_defects

    # Simulation percentages
    sim_pct_clean = ((total_trains - total_defective_trains) / total_trains) * 100
    sim_pct_1 = (trains_with_1_defect / total_trains) * 100
    sim_pct_2 = (trains_with_2_defects / total_trains) * 100
    sim_pct_3_plus = (trains_with_3_plus_defects / total_trains) * 100
    sim_pct_any = (total_defective_trains / total_trains) * 100

    # Print Results
    print("-" * 75)
    print(f"Simulation Results: ORIGINAL METHOD (with Analytical Math Verification)")
    print(f"Total Trains Simulated:    {total_trains:,} ({axles_per_train} axles per train)")
    print(f"Total Axles Simulated:     {total_axles_simulated:,}")
    print(f"Calculated Single Axle Risk (P_axle): {analytics['p_axle']*100:.4f}%")
    print("-" * 75)
    print(f"Total Dice Rolls Executed: {total_dice_rolls:,}")
    print(f"Total Axles with Defects:  {total_defects_triggered:,}")
    
    # Side-by-Side Comparison Table
    print("\nTrain Defect Frequency Breakdown (Simulated vs Analytical Baseline):")
    print(f"  {'Metric':<28} | {'Simulated':<18} | {'Analytical Math':<15}")
    print(f"  {'-'*28}-+-{'-'*18}-+-{'-'*15}")
    print(f"  - Trains with NO defects:  | {total_trains - total_defective_trains:>6,} ({sim_pct_clean:.2f}%)     | {analytics['clean']:.2f}%")
    print(f"  - Trains with 1 defect:    | {trains_with_1_defect:>6,} ({sim_pct_1:.2f}%)     | {analytics['defect_1']:.2f}%")
    print(f"  - Trains with 2 defects:   | {trains_with_2_defects:>6,} ({sim_pct_2:.2f}%)     | {analytics['defect_2']:.2f}%")
    print(f"  - Trains with 3+ defects:  | {trains_with_3_plus_defects:>6,} ({sim_pct_3_plus:.2f}%)     | {analytics['defect_3_plus']:.2f}%")
    print(f"  {'-'*28}-+-{'-'*18}-+-{'-'*15}")
    print(f"  Total trains with defects: | {total_defective_trains:>6,} ({sim_pct_any:.2f}%)     | {analytics['any_defect']:.2f}%")
    
    print("\nDefect Breakdowns (Relative to Total Axles Passed):")
    for name, rate in defects_sorted:
        count = defect_counts[name]
        pct_of_total_axles = (count / total_axles_simulated) * 100 if total_axles_simulated > 0 else 0
        expected_pct = (1.0 / rate) * 100 if rate > 0 else 0
        print(f"  - {name:<20} (1 in every {rate:>5} axles): {count:>6} occurrences ({pct_of_total_axles:.4f}% observed | {expected_pct:.4f}% expected baseline)")
        
    print("\nRelative Distribution (Pie Chart Breakdown among triggered defects):")
    if total_defects_triggered > 0:
        for name, _ in defects_sorted:
            count = defect_counts[name]
            share_pct = (count / total_defects_triggered) * 100
            bar_length = int(share_pct / 2)
            bar = "#" * bar_length
            print(f"  - {name:<20} [{bar:<50}] {share_pct:>6.2f}%")
    print("-" * 75)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Simulate train defect logic with side-by-side analytical metrics.")
    parser.add_argument("-t", "--trains", type=int, default=10000)
    parser.add_argument("-a", "--axles", type=int, default=100)
    args = parser.parse_args()

    config_defects = [
        ("Hot Box ", 100),          
        ("High impact wheel", 200),           
        ("Dragging Equipment", 400) 
    ]

#    config_defects = [
#        ("Hot Box L", 1000),          
#        ("Hot Box R", 1000),          
#        ("Dragging Equipment", 5000), 
#        ("High Car", 5000)           
#    ]

#    config_defects = [
#        ("Hot Box L", 200),          
#        ("Hot Box R", 200),          
#        ("Dragging Equipment", 400), 
#        ("High Car", 2000)           
#    ]

    simulate_with_analytics(args.trains, args.axles, config_defects)