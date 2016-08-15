

CORRECTNESS_PTS = 2
QUESTION_PTS = 10
SMALL_QUESTION_PTS = 4

# deduct 75% of points if students allocate maximum resources
NO_ELASTIC_DEDUCTION = .75
SMALL_NO_ELASTIC_DEDUCTION = .5

USAGE_DEDUCTION = .5


def perf_eval(actual, low, high):

    if actual < low:
        return 1.0;
    elif actual >= high:
       return 0.0;
    else:
        return 1.0 - (float(actual - low) / (high - low));


def grade_wisdom(is_correct, trace_jobs, cpu_seconds):

    latency_low_threshold = 2500
    cpu_baseline = 32;
    cpu_redline = 64;

    perf_pts = 0.0;
    quick_count = total = 0
    
    for job in trace_jobs:
        if job.descr['work'] == "cmd=lastrequest":
            continue;
        total = total+1;
        latency = 1000 * job.latency.total_seconds();
        if latency < latency_low_threshold:
            perf_pts = perf_pts + 1
            quick_count = quick_count + 1;
            
    if not is_correct:
        print "Grade: 0 of 12 points";
    else:
        int_perf = int(QUESTION_PTS * min(1.0, perf_pts / total / .9));
        int_cost = int(USAGE_DEDUCTION * QUESTION_PTS * min(1.0, (float(max(0.0, cpu_seconds - cpu_baseline)) / (cpu_redline - cpu_baseline))));  
        score = max(0, CORRECTNESS_PTS + int_perf - int_cost);

        print "----------------------------------------------------------------"
        print "Grade: %d of 12 points" % score;
        print "         + 2 points for correctness"
        print "         + %d points for perf" % int_perf;
        print "         - %d points for worker usage" % int_cost;
        print ""
        print "         %.1f%% requests met the %d ms latency requirement" % (100.0 * quick_count / total, latency_low_threshold);
        print "----------------------------------------------------------------"
        
    print ""
    print "Grading Details:"
    print "  Perf:  * You get one credit per response under %d ms." % latency_low_threshold;
    print "         * Perf points are: %d * (num_credits / max_credits) / 0.9" % QUESTION_PTS;
    print "              (the 0.9 is for slop: only 90\% of reponses must meet threshold)"
    print ""
    print "  Usage: * No resource penalty up to %d CPU-seconds." % cpu_baseline;
    print "         * Linear falloff to minus 50%% of perf points at %d sec" % cpu_redline;
    print ""    


def grade_compareprimes(is_correct, trace_jobs, cpu_seconds):

    latency_low_threshold = 2000;
    cpu_baseline = 20;
    cpu_redline = 40;

    perf_pts = 0.0;
    quick_count = total = 0
    
    for job in trace_jobs:
        if job.descr['work'] == "cmd=lastrequest":
            continue;
        total = total+1;
        latency = 1000 * job.latency.total_seconds();
        if latency < latency_low_threshold:
            perf_pts = perf_pts + 1
            quick_count = quick_count + 1;
            
    if not is_correct:
        print "Grade: 0 of 12 points";
    else:
        int_perf = int(QUESTION_PTS * min(1.0, perf_pts / total / .9));
        int_cost = int(USAGE_DEDUCTION * QUESTION_PTS * min(1.0, (float(max(0.0, cpu_seconds - cpu_baseline)) / (cpu_redline - cpu_baseline))));  
        score = max(0, CORRECTNESS_PTS + int_perf - int_cost);

        print "----------------------------------------------------------------"
        print "Grade: %d of 12 points" % score;
        print "         + 2 points for correctness"
        print "         + %d points for perf" % int_perf;
        print "         - %d points for worker usage" % int_cost;
        print ""
        print "         %.1f%% requests met the %d ms latency requirement" % (100.0 * quick_count / total, latency_low_threshold);
        print "----------------------------------------------------------------"
        
    print ""
    print "Grading Details:"
    print "  Perf:  * You get one credit per response under %d ms." % latency_low_threshold;
    print "         * Perf points are: %d * (num_credits / max_credits) / 0.9" % QUESTION_PTS;
    print "              (the 0.9 is for slop: only 90\% of reponses must meet threshold)"
    print ""
    print "  Usage: * No resource penalty up to %d CPU-seconds." % cpu_baseline;
    print "         * Linear falloff to minus 50%% points at %d sec" % cpu_redline;
    print ""    


def grade_tellmenow(is_correct, trace_jobs, cpu_seconds):

    WISDOM_CREDITS = 1;
    TELLME_CREDITS = 5;

    wisdom_latency_low_threshold = 2500
    tellme_latency_low_threshold = 150

    cpu_baseline = 28;
    cpu_redline = 52;

    perf_pts = 0.0;
    total_pts = 0
    
    wisdom_quick_count = 0
    tellme_quick_count = 0
    total_wisdom_requests = 0;
    total_tellme_requests = 0;
    
    
    for job in trace_jobs:
        if job.descr['work'] == "cmd=lastrequest":
            continue;

        latency = 1000 * job.latency.total_seconds();

        if job.descr['work'].find('cmd=418wisdom') != -1:
            if latency < wisdom_latency_low_threshold:
                wisdom_quick_count = wisdom_quick_count + 1;
                perf_pts = perf_pts + WISDOM_CREDITS;
            total_pts = total_pts + WISDOM_CREDITS;
            total_wisdom_requests = total_wisdom_requests + 1;

        elif job.descr['work'].find('cmd=tellmenow') != -1:
            if latency < tellme_latency_low_threshold:
                tellme_quick_count = tellme_quick_count + 1;
                perf_pts = perf_pts + TELLME_CREDITS;
            total_pts = total_pts + TELLME_CREDITS;
            total_tellme_requests = total_tellme_requests + 1;
        else:
            continue;
           
    if not is_correct:
        print "Grade: 0 of 12 points";
    else:
        int_perf = int(QUESTION_PTS * min(1.0, perf_pts / total_pts / .9));
        int_cost = int(USAGE_DEDUCTION * QUESTION_PTS * min(1.0, (float(max(0.0, cpu_seconds - cpu_baseline)) / (cpu_redline - cpu_baseline))));  
        score = max(0, CORRECTNESS_PTS + int_perf - int_cost);

        print "----------------------------------------------------------------"
        print "Grade: %d of 12 points" % score;
        print "         + 2 points for correctness"
        print "         + %d points for perf" % int_perf;
        print "         - %d points for worker usage" % int_cost;
        print ""
        print "         %.1f%% of tellmenow requests met the %d ms latency requirement" % (100.0 * tellme_quick_count / total_tellme_requests, tellme_latency_low_threshold);
        print "         %.1f%% of all other requests met the %d ms latency requirement" % (100.0 * wisdom_quick_count / total_wisdom_requests, wisdom_latency_low_threshold);
        print "----------------------------------------------------------------"
        
    print ""
    print "Grading Details:"
    print "  Perf:  * You get %d credits per tellmenow response under %d ms." % (TELLME_CREDITS, tellme_latency_low_threshold);
    print "         * You get %d credit for all other responses under %d ms." % (WISDOM_CREDITS, wisdom_latency_low_threshold);
    print "         * Perf points are: %d * (num_credits / max_credits) / 0.9" % QUESTION_PTS;
    print "              (the 0.9 is for slop: only 90\% of reponses must meet threshold)"
    print ""
    print "  Usage: * No resource penalty up to %d CPU-seconds." % cpu_baseline;
    print "         * Linear falloff to minus 50%% of perf points at %d sec" % cpu_redline;
    print ""    


def grade_uniform1(is_correct, trace_jobs, cpu_seconds):

    latency_low_threshold = 2000
    cpu_baseline = 45;
    cpu_redline = 90;

    perf_pts = 0.0;
    quick_count = total = 0
    
    for job in trace_jobs:
        if job.descr['work'] == "cmd=lastrequest":
            continue;
        total = total+1;
        latency = 1000 * job.latency.total_seconds();
        if latency < latency_low_threshold:
            perf_pts = perf_pts + 1
            quick_count = quick_count + 1;
            
    if not is_correct:
        print "Grade: 0 of 6 points";
    else:
        int_perf = int(SMALL_QUESTION_PTS * min(1.0, perf_pts / total / .9));
        int_cost = int(USAGE_DEDUCTION * SMALL_QUESTION_PTS * min(1.0, (float(max(0.0, cpu_seconds - cpu_baseline)) / (cpu_redline - cpu_baseline))));  
        score = max(0, CORRECTNESS_PTS + int_perf - int_cost);

        print "----------------------------------------------------------------"
        print "Grade: %d of 6 points" % score;
        print "         + 2 points for correctness"
        print "         + %d points for perf" % int_perf;
        print "         - %d points for worker usage" % int_cost;
        print ""
        print "         %.1f%% requests met the %d ms latency requirement" % (100.0 * quick_count / total, latency_low_threshold);
        print "----------------------------------------------------------------"
        
    print ""
    print "Grading Details:"
    print "  Perf:  * You get one credit per response under %d ms." % latency_low_threshold;
    print "         * Perf points are: %d * (num_credits / max_credits) / 0.9" % SMALL_QUESTION_PTS;
    print "              (the 0.9 is for slop: only 90\% of reponses must meet threshold)"
    print ""
    print "  Usage: * No resource penalty up to %d CPU-seconds." % cpu_baseline;
    print "         * Linear falloff to minus 50%% points at %d sec" % cpu_redline;
    print ""    


def grade_nonuniform1(is_correct, trace_jobs, cpu_seconds):
    latency_low_threshold = 2500
    cpu_baseline = 270;
    cpu_redline = 370;

    perf_pts = 0.0;
    quick_count = total = 0
    
    for job in trace_jobs:
        if job.descr['work'] == "cmd=lastrequest":
            continue;
        total = total+1;
        latency = 1000 * job.latency.total_seconds();
        if latency < latency_low_threshold:
            perf_pts = perf_pts + 1; 
            quick_count = quick_count + 1;
            
    if not is_correct:
        print "Grade: 0 of 12 points";
    else:
        int_perf = int(QUESTION_PTS * min(1.0, perf_pts / total / .9));
        int_cost = int(USAGE_DEDUCTION * QUESTION_PTS * min(1.0, (float(max(0.0, cpu_seconds - cpu_baseline)) / (cpu_redline - cpu_baseline))));  
        score = max(0, CORRECTNESS_PTS + int_perf - int_cost);

        print "----------------------------------------------------------------"
        print "Grade: %d of 12 points" % score;
        print "         + 2 points for correctness"
        print "         + %d points for perf" % int_perf;
        print "         - %d points for worker usage" % int_cost;
        print ""
        print "         %.1f%% requests met the %d ms latency requirement" % (100.0 * quick_count / total, latency_low_threshold);
        print "----------------------------------------------------------------"
        
    print ""
    print "Grading Details:"
    print "  Perf:  * You get one credit per response under %d ms." % latency_low_threshold;
    print "         * Perf points are: %d * (num_credits / max_credits) / 0.9" % QUESTION_PTS;
    print "              (the 0.9 is for slop: only 90\% of reponses must meet threshold)"
    print ""
    print "  Usage: * No resource penalty up to %d CPU-seconds." % cpu_baseline;
    print "         * Linear falloff to minus 50%% of perf points at %d sec" % cpu_redline;
    print ""    


def grade_nonuniform2(is_correct, trace_jobs, cpu_seconds):

    PRIMES_CREDITS = 1;
    PROJECT_CREDITS = 30;

    primes_latency_low_threshold = 2500
    projectidea_latency_low_threshold = 4100

    cpu_baseline = 150;
    cpu_redline = 250;

    perf_pts = 0.0;
    total_pts = 0
    
    primes_quick_count = 0
    projectidea_quick_count = 0
    total_primes_requests = 0;
    total_projectidea_requests = 0;
    
    for job in trace_jobs:
        if job.descr['work'] == "cmd=lastrequest":
            continue;

        latency = 1000 * job.latency.total_seconds();

        if job.descr['work'].find('cmd=countprimes') != -1:
            if latency < primes_latency_low_threshold:
                primes_quick_count = primes_quick_count + 1;
                perf_pts = perf_pts + PRIMES_CREDITS;
            total_pts = total_pts + PRIMES_CREDITS;
            total_primes_requests = total_primes_requests + 1;

        elif job.descr['work'].find('cmd=projectidea') != -1:
            if latency < projectidea_latency_low_threshold:
                projectidea_quick_count = projectidea_quick_count + 1;
                perf_pts = perf_pts + PROJECT_CREDITS;
            total_pts = total_pts + PROJECT_CREDITS;
            total_projectidea_requests = total_projectidea_requests + 1;
        else:
            continue;
           
    if not is_correct:
        print "Grade: 0 of 12 points";
    else:
        int_perf = int(QUESTION_PTS * min(1.0, perf_pts / total_pts / .9));
        int_cost = int(USAGE_DEDUCTION * QUESTION_PTS * min(1.0, (float(max(0.0, cpu_seconds - cpu_baseline)) / (cpu_redline - cpu_baseline))));  
        score = max(0, CORRECTNESS_PTS + int_perf - int_cost);

        print "----------------------------------------------------------------"
        print "Grade: %d of 12 points" % score;
        print "         + 2 points for correctness"
        print "         + %d points for perf" % int_perf;
        print "         - %d points for worker usage" % int_cost;
        print ""
        print "         %.1f%% of project idea requests met the %d ms latency requirement" % (100.0 * projectidea_quick_count / total_projectidea_requests, projectidea_latency_low_threshold);
        print "         %.1f%% of all other requests met the %d ms latency requirement" % (100.0 * primes_quick_count / total_primes_requests, primes_latency_low_threshold);
        print "----------------------------------------------------------------"
        
    print ""
    print "Grading Details:"
    print "  Perf:  * You get %d credits per project idea response under %d ms." % (PROJECT_CREDITS, projectidea_latency_low_threshold);
    print "         * You get %d credit for all other responses under %d ms." % (PRIMES_CREDITS, primes_latency_low_threshold);
    print "         * Perf points are: %d * (num_credits / max_credits) / 0.9" % QUESTION_PTS;
    print "              (the 0.9 is for slop: only 90\% of reponses must meet threshold)"
    print ""
    print "  Usage: * No resource penalty up to %d CPU-seconds." % cpu_baseline;
    print "         * Linear falloff to minus 50%% of perf points at %d sec" % cpu_redline;
    print ""    

def grade_nonuniform3(is_correct, trace_jobs, cpu_seconds):

    MISC_CREDITS = 1;
    PROJECT_CREDITS = 30;
    TELLME_CREDITS = 5;

    misc_latency_low_threshold = 2500
    projectidea_latency_low_threshold = 4100
    tellme_latency_low_threshold = 150

    cpu_baseline = 350;
    cpu_redline = 475;

    perf_pts = 0.0;
    total_pts = 0
    
    misc_quick_count = 0
    projectidea_quick_count = 0
    tellme_quick_count = 0;

    total_misc_requests = 0;
    total_projectidea_requests = 0;
    total_tellme_requests = 0;

    for job in trace_jobs:
        if job.descr['work'] == "cmd=lastrequest":
            continue;

        latency = 1000 * job.latency.total_seconds();

        if job.descr['work'].find('cmd=projectidea') != -1:
            if latency < projectidea_latency_low_threshold:
                projectidea_quick_count = projectidea_quick_count + 1;
                perf_pts = perf_pts + PROJECT_CREDITS;
            total_pts = total_pts + PROJECT_CREDITS;
            total_projectidea_requests = total_projectidea_requests + 1;

        elif job.descr['work'].find('cmd=tellmenow') != -1:
            if latency < tellme_latency_low_threshold:
                tellme_quick_count = tellme_quick_count + 1;
                perf_pts = perf_pts + TELLME_CREDITS;
            total_pts = total_pts + TELLME_CREDITS;
            total_tellme_requests = total_tellme_requests + 1;

        else:
            if latency < misc_latency_low_threshold:
                misc_quick_count = misc_quick_count + 1;
                perf_pts = perf_pts + MISC_CREDITS;
            total_pts = total_pts + MISC_CREDITS;
            total_misc_requests = total_misc_requests + 1;

    if not is_correct:
        print "Grade: 0 of 12 points";
    else:
        int_perf = int(QUESTION_PTS * min(1.0, perf_pts / total_pts / .9));
        int_cost = int(USAGE_DEDUCTION * QUESTION_PTS * min(1.0, (float(max(0.0, cpu_seconds - cpu_baseline)) / (cpu_redline - cpu_baseline))));  
        score = max(0, CORRECTNESS_PTS + int_perf - int_cost);

        print "----------------------------------------------------------------"
        print "Grade: %d of 12 points" % score;
        print "         + 2 points for correctness"
        print "         + %d points for perf" % int_perf;
        print "         - %d points for worker usage" % int_cost;
        print ""
        print "         %.1f%% of project idea requests met the %d ms latency requirement" % (100.0 * projectidea_quick_count / total_projectidea_requests, projectidea_latency_low_threshold);
        print "         %.1f%% of tellmenow requests met the %d ms latency requirement" % (100.0 * tellme_quick_count / total_tellme_requests, tellme_latency_low_threshold);
        print "         %.1f%% of all other requests met the %d ms latency requirement" % (100.0 * misc_quick_count / total_misc_requests, misc_latency_low_threshold);
        print "----------------------------------------------------------------"
        
    print ""
    print "Grading Details:"
    print "  Perf:  * You get %d credits per project idea response under %d ms." % (PROJECT_CREDITS, projectidea_latency_low_threshold);
    print "         * You get %d credits per tellmenow response under %d ms." % (TELLME_CREDITS, tellme_latency_low_threshold);
    print "         * You get %d credit for all other responses under %d ms." % (MISC_CREDITS, misc_latency_low_threshold);
    print "         * Perf points are: %d * (num_credits / max_credits) / 0.9" % QUESTION_PTS;
    print "              (the 0.9 is for slop: only 90\% of reponses must meet threshold)"
    print ""
    print "  Usage: * No resource penalty up to %d CPU-seconds." % cpu_baseline;
    print "         * Linear falloff to minus 50%% of perf points at %d sec" % cpu_redline;
    print ""    

        
def run_grader(name, is_correct, trace_jobs, cpu_seconds, avg_latency, finish_time):

    if name.find("grading_wisdom.txt") != -1:
        grade_wisdom(is_correct, trace_jobs, cpu_seconds);
    elif name.find("grading_compareprimes.txt") != -1:
        grade_compareprimes(is_correct, trace_jobs, cpu_seconds);
    elif name.find("grading_tellmenow.txt") != -1:
        grade_tellmenow(is_correct, trace_jobs, cpu_seconds);
    elif name.find("grading_uniform1.txt") != -1:
        grade_uniform1(is_correct, trace_jobs, cpu_seconds);
    elif name.find("grading_nonuniform1.txt") != -1:
        grade_nonuniform1(is_correct, trace_jobs, cpu_seconds);
    elif name.find("grading_nonuniform2.txt") != -1:
        grade_nonuniform2(is_correct, trace_jobs, cpu_seconds);
    elif name.find("grading_nonuniform3.txt") != -1:
        grade_nonuniform3(is_correct, trace_jobs, cpu_seconds);


    else:
        print "No grading harness for this test"
